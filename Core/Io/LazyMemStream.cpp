#include "stdafx.h"
#include "LazyMemStream.h"
#include "CloneEnv.h"

namespace storm {

	LazyMemIStream::LazyMemIStream(IStream *src) {
		data = buffer(engine(), 1024);
	}

	LazyMemIStream::LazyMemIStream(const LazyMemIStream &o) : data(o.data) {}

	void LazyMemIStream::deepCopy(CloneEnv *env) {
		src = clone(src, env);
		// We never change the data, so no need to clone it!
	}

	Bool LazyMemIStream::more() {
		return pos < data.count() || src->more();
	}

	Buffer LazyMemIStream::read(Buffer to) {
		Nat start = to.filled();
		to = peek(to);
		pos += to.filled() - start;
		return to;
	}

	Buffer LazyMemIStream::peek(Buffer to) {
		if (pos >= data.filled())
			fill();

		Nat start = to.filled();
		Nat copy = min(to.count() - start, data.filled() - pos);
		memcpy(to.dataPtr() + start, data.dataPtr() + pos, copy);
		to.filled(copy + start);
		return to;
	}

	void LazyMemIStream::seek(Word to) {
		Nat npos = Nat(to);

		// Read data until we have the requested address, or until nothing remains of the source stream.
		while (npos > data.filled() && src->more())
			fill();

		// Clip so that we're not outside the allowed region.
		pos = min(npos, data.filled());
	}

	Word LazyMemIStream::tell() {
		return pos;
	}

	Word LazyMemIStream::length() {
		// This is the best we can do.
		return data.filled();
	}

	RIStream *LazyMemIStream::randomAccess() {
		return this;
	}

	void LazyMemIStream::toS(StrBuf *to) const {
		data.outputMark(to, pos);
	}

	void LazyMemIStream::fill() {
		if (!src->more())
			return;

		if (data.full()) {
			// Stop doubling after this size (currently 1 MB)
			const Nat cutoff = 1 * 1024 * 1024;

			// Grow the buffer.
			Nat newSize;
			if (data.count() < cutoff) {
				newSize = data.count() * 2;
			} else {
				newSize = data.count() + cutoff;
			}

			Buffer n = buffer(engine(), newSize);
			n.filled(data.count());

			// Copy the old data.
			memcpy(n.dataPtr(), data.dataPtr(), data.count());
			data = n;
		}

		// Try to read as much as possible. Since we have memory, we might as well use it!
		// The "read" operation will not necessarily fill up the buffer.
		src->read(data);
	}

}
