#include "stdafx.h"
#include "MemStream.h"
#include "Core/CloneEnv.h"
#include "Core/StrBuf.h"

namespace storm {

	MemIStream::MemIStream(Buffer b) : data(buffer(engine(), b.filled())), pos(0) {
		// Copy the filled data.
		memcpy(data.dataPtr(), b.dataPtr(), b.filled());
		data.filled(b.filled());
	}

	MemIStream::MemIStream(const MemIStream &o) : data(o.data) {}

	void MemIStream::deepCopy(CloneEnv *env) {
		// We never change 'data', so we do not need to clone it here.
	}

	Bool MemIStream::more() {
		return pos < data.count();
	}

	Buffer MemIStream::read(Buffer to) {
		Nat start = to.filled();
		to = peek(to);
		pos += to.filled() - start;
		return to;
	}

	Buffer MemIStream::peek(Buffer to) {
		Nat start = to.filled();
		Nat copy = min(to.count() - start, data.count() - pos);
		memcpy(to.dataPtr() + start, data.dataPtr() + pos, copy);
		to.filled(copy + start);
		return to;
	}

	void MemIStream::seek(Word to) {
		pos = min(nat(to), data.count());
	}

	Word MemIStream::tell() {
		return pos;
	}

	Word MemIStream::length() {
		return data.count();
	}

	RIStream *MemIStream::randomAccess() {
		return this;
	}

	void MemIStream::toS(StrBuf *to) const {
		data.outputMark(to, pos);
	}


	MemOStream::MemOStream() {}

	MemOStream::MemOStream(Buffer appendTo) : data(appendTo) {}

	MemOStream::MemOStream(const MemOStream &o) : data(o.data) {
		data.deepCopy(null);
	}

	void MemOStream::deepCopy(CloneEnv *env) {
		// Nothing to do...
	}

	Nat MemOStream::write(Buffer src, Nat start) {
		start = min(start, src.filled());
		Nat copy = src.filled() - start;
		Nat filled = data.filled();
		if (filled + copy >= data.count()) {
			// Grow the buffer...
			Nat growTo = max(max(data.count() * 2, copy), Nat(1024));
			data = grow(engine(), data, growTo);
		}

		// Copy data.
		memcpy(data.dataPtr() + filled, src.dataPtr() + start, copy);
		data.filled(filled + copy);

		return copy;
	}

	Buffer MemOStream::buffer() {
		return data;
	}

	void MemOStream::toS(StrBuf *to) const {
		*to << data;
	}

}
