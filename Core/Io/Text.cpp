#include "stdafx.h"
#include "Text.h"
#include "Utf8Text.h"
#include "Utf16Text.h"
#include "StrInput.h"
#include "Core/Str.h"
#include "Core/StrBuf.h"

namespace storm {

	TextInfo::TextInfo() : useCrLf(false), useBom(false) {}

	TextInfo sysTextInfo() {
#ifdef WINDOWS
		return windowsTextInfo();
#else
		return unixTextInfo();
#endif
	}

	TextInfo unixTextInfo() {
		TextInfo info;
		return info;
	}

	TextInfo windowsTextInfo() {
		TextInfo info;
		info.useCrLf = true;
		return info;
	}

	TextInfo windowsTextInfo(Bool bom) {
		TextInfo info;
		info.useCrLf = true;
		info.useBom = bom;
		return info;
	}

	TextInput::TextInput() : next(nat(0)), hasNext(false), first(true), eof(false) {}

	Char TextInput::doRead() {
		Char c = readChar();
		if (first && c == Char(Nat(0xFEFF)))
			c = readChar();
		if (c == Char(Nat(0)))
			eof = true;
		return c;
	}

	Char TextInput::read() {
		Char r = readRaw();
		if (r == Char('\r'))
			if (peek() == Char('\n'))
				r = readRaw();
		return r;
	}

	Char TextInput::readRaw() {
		if (hasNext) {
			hasNext = false;
			return next;
		} else {
			return doRead();
		}
	}

	Char TextInput::peek() {
		if (!hasNext) {
			next = doRead();
			hasNext = true;
		}
		return next;
	}

	Bool TextInput::more() {
		if (hasNext)
			return next != Char(Nat(0));
		return !eof;
	}

	Str *TextInput::readLine() {
		StrBuf *to = new (this) StrBuf();

		while (true) {
			Char c = readRaw();
			if (c == Char(nat(0))) {
				break;
			}
			if (c == Char('\r')) {
				c = peek();
				if (c == Char('\n'))
					readRaw();
				break;
			}
			if (c == Char('\n')) {
				break;
			}
			*to << c;
		}

		return to->toS();
	}

	Str *TextInput::readAll() {
		StrBuf *to = new (this) StrBuf();

		while (true) {
			Char c = readRaw();
			if (c == Char(Nat(0)))
				break;
			if (c == Char('\r')) {
				c = read();
				// Single \r?
				if (c != Char('\n'))
					*to << L"\n";
			}
			*to << c;
		}

		return to->toS();
	}

	Str *TextInput::readAllRaw() {
		StrBuf *to = new (this) StrBuf();

		while (more()) {
			Char c = readRaw();
			if (c == Char(Nat(0)))
				break;
			*to << c;
		}

		return to->toS();
	}

	void TextInput::close() {}

	// Char TextInput::readChar() {
	// 	return Char(nat(0));
	// }


	TextInput *STORM_FN readText(IStream *stream) {
		nat16 bom = 0;

		Buffer buf = stream->fill(sizeof(bom));
		if (buf.full()) {
			// Read in network order.
			bom = buf[0] << 8;
			bom |= buf[1];
		}

		if (bom == 0xFEFF)
			return new (stream) Utf16Input(stream, false, buf);
		else if (bom == 0xFFFE)
			return new (stream) Utf16Input(stream, true, buf);
		else
			return new (stream) Utf8Input(stream, buf);
	}

	TextInput *STORM_FN readText(Url *from) {
		return readText(from->read());
	}

	TextInput *STORM_FN readStr(Str *from) {
		return new (from) StrInput(from);
	}

	Str *STORM_FN readAllText(Url *file) {
		IStream *src = file->read();
		Str *result = readText(src)->readAll();
		src->close();
		return result;
	}


	/**
	 * Write text.
	 */

	TextOutput::TextOutput() : autoFlush(true), config() {}

	TextOutput::TextOutput(TextInfo info) : autoFlush(true), config(info) {}

	void TextOutput::write(Char c) {
		writeBom();
		writeChar(c);
	}

	void TextOutput::write(Str *s) {
		writeBom();

		Char newline('\n');
		for (Str::Iter i = s->begin(), end = s->end(); i != end; ++i) {
			if (i.v() == newline)
				writeLine();
			else
				writeChar(i.v());
		}
	}

	void TextOutput::writeLine() {
		writeBom();

		if (config.useCrLf)
			writeChar(Char('\r'));
		writeChar(Char('\n'));
		if (autoFlush)
			flush();
	}

	void TextOutput::writeLine(Str *s) {
		write(s);
		writeLine();
	}

	void TextOutput::writeBom() {
		if (config.useBom) {
			writeChar(Char(Nat(0xFEFF)));
			config.useBom = false;
		}
	}

	// void TextOutput::writeChar(Char ch) {}

	void TextOutput::flush() {}

	void TextOutput::close() {}

}
