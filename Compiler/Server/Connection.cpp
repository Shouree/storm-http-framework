#include "stdafx.h"
#include "Connection.h"
#include "Core/Timing.h"
#include "Core/StrBuf.h"
#include "Core/Io/Utf8Text.h"
#include "Core/Io/MemStream.h"

namespace storm {
	namespace server {

		Connection::Connection(IStream *input, OStream *output)
			: input(input), output(output) {

			TextInfo info;
			info.useCrLf = false;
			info.useBom = false;
			textOut = new (this) Utf8Output(output, info);
			symNames = new (this) NameMap();
			symIds = new (this) IdMap();
			lastSymId = 0x40000000; // Emacs only uses ~30 bits for integers.
			inputBuffer = new (this) BufStream();
		}

		Symbol *Connection::symbol(const wchar *name) {
			return symbol(new (this) Str(name));
		}

		Symbol *Connection::symbol(Str *name) {
			NameMap::Iter i = symNames->find(name);
			if (i == symNames->end()) {
				Symbol *sym = new (this) Symbol(name, --lastSymId);
				symNames->put(name, sym);
				return sym;
			} else {
				return i.v();
			}
		}

		Symbol *Connection::symbol(Nat id) {
			IdMap::Iter i = symIds->find(id);
			if (i == symIds->end()) {
				return null;
			} else {
				return i.v();
			}
		}

		Bool Connection::sendSymbol(Symbol *sym) {
			if (symIds->has(sym->id))
				return false;

			symIds->put(sym->id, sym);
			return true;
		}

		void Connection::send(SExpr *expr) {
			MemOStream *out = new (this) MemOStream();

			// Leading zero byte.
			GcPreArray<byte, 5> d;
			d.v[0] = 0x00;
			d.v[1] = 0x00;
			d.v[2] = 0x00;
			d.v[3] = 0x00;
			d.v[4] = 0x00;
			out->write(fullBuffer(d));

			// Rest of the message.
			write(out, expr);

			// Update the length of the message.
			Buffer b = out->buffer();
			Nat v = b.filled() - 5;
			b[1] = byte((v >> 24) & 0xFF);
			b[2] = byte((v >> 16) & 0xFF);
			b[3] = byte((v >>  8) & 0xFF);
			b[4] = byte((v >>  0) & 0xFF);

			// Send the message.
			// textOut->writeLine(expr->toS());
			// textOut->writeLine(out->toS());
			output->write(b);
		}

		void Connection::write(OStream *to, MAYBE(SExpr *) expr) {
			if (expr) {
				expr->write(to, this);
			} else {
				GcPreArray<byte, 1> d;
				d.v[0] = 0x00;
				to->write(fullBuffer(d));
			}
		}

		SExpr *Connection::receive() {
			// Read messages from the buffer until we fail (all failures are due to not enough data).
			SExpr *result = null;
			while (result == null) {
				if (!read(result)) {
					// We need more data before we can try again.
					if (!fillBuffer()) {
						// Reached EOF...
						return null;
					}
				}
			}

			return result;
		}

		static Byte decodeByte(IStream *src, Bool &ok) {
			GcPreArray<byte, 1> buf;
			Buffer r = src->readAll(emptyBuffer(buf));

			if (!r.full()) {
				ok = false;
				return 0;
			}

			return r[0];
		}

		static Byte peekByte(IStream *src, Bool &ok) {
			GcPreArray<byte, 1> buf;
			Buffer r = src->peek(emptyBuffer(buf));

			if (!r.full()) {
				ok = false;
				return 0;
			}

			return r[0];
		}

		static Nat decodeNat(IStream *src, Bool &ok) {
			GcPreArray<byte, 4> buf;
			Buffer r = src->readAll(emptyBuffer(buf));

			if (!r.full()) {
				ok = false;
				return 0;
			}

			return (Nat(r[0]) << 24)
				| (Nat(r[1]) << 16)
				| (Nat(r[2]) << 8)
				| (Nat(r[3]) << 0);
		}

		Bool Connection::read(SExpr *&result) {
			BufStream *from = inputBuffer;

			Bool ok = true;
			Byte first = peekByte(from, ok);

			if (!ok) {
				// Nothing to read...
				return false;
			}

			if (first == 0x00) {
				// textOut->writeLine(new (this) Str(L"Reading a message..."));

				// This is an SExpr, try to parse it!
				Word pos = from->tell();

				// Skip the 0x00 byte.
				from->seek(pos + 1);

				// PVAR(from);

				// See if we have enough data available.
				Nat len = decodeNat(from, ok);
				SExpr *r = null;
				if (ok) {
					if (from->tell() + len <= from->length()) {
						r = readSExpr(from, ok);
					} else {
						ok = false;
					}
				}

				if (ok) {
					result = r;
				} else {
					// Make sure we re-try next time!
					from->seek(pos);
				}
			} else {
				// Plain text. Extract as much as possible to stdin.
				Nat len = from->findByte(0x00);
				Buffer fwd = from->read(len);
				// TODO: Forward 'fwd' to stdin!
				UNUSED(fwd);
			}

			return ok;
		}

		Bool Connection::fillBuffer() {
			const Nat chunk = 6*1024;
			Buffer r = input->read(chunk);
			if (r.empty()) {
				// EOF reached...
				// Note: Ctrl+D on UNIX will also end up here.
				return false;
			}

			inputBuffer->append(r);
			return true;
		}

		MAYBE(SExpr *) Connection::readSExpr(IStream *from, Bool &ok) {
			if (!ok)
				return null;

			Byte kind = decodeByte(from, ok);
			switch (kind) {
			case SExpr::nil:
				return null;
			case SExpr::cons:
				return readCons(from, ok);
			case SExpr::number:
				return readNumber(from, ok);
			case SExpr::string:
				return readString(from, ok);
			case SExpr::newSymbol:
				return readNewSymbol(from, ok);
			case SExpr::oldSymbol:
				return readOldSymbol(from, ok);
			default:
				// Return null for this sub-expression, so that parsing goes on...
				textOut->writeLine(new (this) Str(L"WARNING: Invalid type found in message."));
				return null;
			}
		}

		MAYBE(SExpr *) Connection::readCons(IStream *from, Bool &ok) {
			Cons *first = new (this) Cons(readSExpr(from, ok), null);
			Cons *curr = first;
			while (ok && peekByte(from, ok) == SExpr::cons) {
				// Consume the byte we peeked.
				decodeByte(from, ok);

				Cons *t = new (this) Cons(readSExpr(from, ok), null);
				curr->rest = t;
				curr = t;
			}

			if (!ok)
				return null;

			// Read the last cell as well.
			curr->rest = readSExpr(from, ok);
			return first;
		}

		MAYBE(SExpr *) Connection::readNumber(IStream *from, Bool &ok) {
			Nat r = decodeNat(from, ok);
			if (!ok)
				return null;

			return new (this) Number(Int(r));
		}

		MAYBE(SExpr *) Connection::readString(IStream *from, Bool &ok) {
			Nat len = decodeNat(from, ok);
			if (!ok)
				return null;

			Buffer str = from->readAll(len);
			if (str.filled() < len) {
				ok = false;
				return null;
			}

			MemIStream *src = new (this) MemIStream(str);
			TextInput *text = new (this) Utf8Input(src);
			Str *data = text->readAllRaw();

			return new (this) String(data);
		}

		MAYBE(SExpr *) Connection::readNewSymbol(IStream *from, Bool &ok) {
			Nat symId = decodeNat(from, ok);
			Nat len = decodeNat(from, ok);
			if (!ok)
				return null;

			Buffer str = from->readAll(len);
			if (str.filled() < len) {
				ok = false;
				return null;
			}

			MemIStream *src = new (this) MemIStream(str);
			TextInput *text = new (this) Utf8Input(src);
			Str *name = text->readAllRaw();

			// Insert the symbol if it does not already exist.
			Symbol *sym = null;
			if (symNames->has(name)) {
				sym = symNames->get(name);
			} else {
				sym = new (this) Symbol(name, symId);
				symNames->put(name, sym);
			}
			symIds->put(symId, sym);

			return sym;
		}

		MAYBE(SExpr *) Connection::readOldSymbol(IStream *from, Bool &ok) {
			Nat symId = decodeNat(from, ok);
			if (!ok)
				return null;

			Symbol *sym = symIds->get(symId, null);
			// Return 'sym' even if it is null, so that we can proceed reading input after the
			// corrupt data.
			return sym;
		}

	}
}
