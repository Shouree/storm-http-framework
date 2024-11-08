#include "stdafx.h"
#include "PeekStream.h"

namespace storm {

	static const GcType bufType = {
		GcType::tArray,
		null,
		null,
		sizeof(byte),
		0,
		{}
	};

	PeekIStream::PeekIStream() : lookahead(null), lookaheadStart(0), atEof(false) {}

	PeekIStream::PeekIStream(const PeekIStream &o) :  lookahead(null), lookaheadStart(0), atEof(false) {
		// Copy the relevant parts of 'o.lookahead':
		nat toCopy = o.lookaheadAvail();
		if (toCopy > 0) {
			ensureLookahead(toCopy);
			memcpy(lookahead->v, o.lookahead->v + o.lookaheadStart, toCopy);
		}
	}

	Bool PeekIStream::more() {
		if (lookaheadAvail() > 0)
			return true;

		return !atEof;
	}

	Buffer PeekIStream::read(Buffer b) {
		Nat start = b.filled();
		Nat read = b.count() - start;

		// Nothing to read?
		if (read == 0)
			return b;

		// Is there anything left in the lookahead for us to consume?
		Nat avail = lookaheadAvail();
		if (avail > 0) {
			Nat copy = min(read, avail);
			memcpy(b.dataPtr() + start, lookahead->v + lookaheadStart, copy);
			lookaheadStart += copy;
			start += copy;
			read -= copy;
			b.filled(start);
		}

		// Done reading from the lookahead alone?
		if (read == 0)
			return b;

		PeekReadResult r = doRead(b.dataPtr() + start, read);
		if (r.bytesRead() == 0)
			atEof = !r.timedOut();
		b.filled(r.bytesRead() + start);
		return b;
	}

	Buffer PeekIStream::peek(Buffer b) {
		Nat start = b.filled();
		Nat toPeek = b.count() - start;
		Nat avail = doLookahead(toPeek);
		if (!lookahead)
			return b;

		toPeek = min(toPeek, avail);
		memcpy(b.dataPtr() + start, lookahead->v + lookaheadStart, toPeek);
		b.filled(toPeek + start);

		return b;
	}

	Nat PeekIStream::lookaheadAvail() const {
		if (!lookahead)
			return 0;

		return Nat(lookahead->filled) - lookaheadStart;
	}

	Nat PeekIStream::doLookahead(Nat bytes) {
		Nat avail = lookaheadAvail();
		if (avail >= bytes)
			return avail;

		ensureLookahead(bytes);

		// We need to read more data!
		Nat read = bytes - avail;
		PeekReadResult more = doRead(lookahead->v + lookahead->filled, read);
		lookahead->filled += more.bytesRead();
		if (more.bytesRead() == 0)
			atEof = !more.timedOut();

		return lookaheadAvail();
	}

	void PeekIStream::ensureLookahead(Nat n) {
		if (!lookahead) {
			// Easy, just allocate a new buffer.
			lookahead = runtime::allocArray<byte>(engine(), &bufType, n);
			return;
		}

		if (lookahead->count - lookaheadStart >= n) {
			// Enough space!
			return;
		}

		// We need to re-allocate...
		Nat preserve = Nat(lookahead->filled) - lookaheadStart;
		GcArray<byte> *t = runtime::allocArray<byte>(engine(), &bufType, n);
		t->filled = preserve;
		memcpy(t->v, lookahead->v + lookaheadStart, preserve);
		lookahead = t;
		lookaheadStart = 0;
	}

	void PeekIStream::close() {
		lookahead = null;
		lookaheadStart = 0;
	}

	PeekReadResult PeekIStream::doRead(byte *to, Nat count) {
		return PeekReadResult::end();
	}


}
