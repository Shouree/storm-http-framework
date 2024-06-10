#include "stdafx.h"
#include "BufStream.h"
#include "Core/CloneEnv.h"

namespace storm {
	namespace server {

		BufStream::BufStream() {}

		BufStream::BufStream(const BufStream &o) : data(o.data), pos(o.pos) {}

		void BufStream::deepCopy(CloneEnv *env) {
			cloned(data, env);
		}

		Bool BufStream::more() {
			return pos < data.filled();
		}

		Buffer BufStream::read(Buffer to) {
			Nat start = to.filled();
			Nat copy = min(to.count() - to.filled(), data.filled() - pos);
			memcpy(to.dataPtr() + start, data.dataPtr() + pos, copy);
			pos += copy;
			to.filled(start + copy);
			return to;
		}

		Buffer BufStream::peek(Buffer to) {
			Nat oldPos = pos;
			Buffer r = read(to);
			pos = oldPos;
			return r;
		}

		void BufStream::seek(Word to) {
			pos = min(Nat(to), data.filled());
		}

		Word BufStream::tell() {
			return pos;
		}

		Word BufStream::length() {
			return data.filled();
		}

		RIStream *BufStream::randomAccess() {
			return this;
		}

		void BufStream::toS(StrBuf *to) const {
			data.outputMark(to, pos);
		}

		Nat BufStream::findByte(Byte b) const {
			Nat count = data.filled();
			for (Nat i = pos; i < count; i++)
				if (data[i] == b)
					return i - pos;
			return count - pos;
		}

		void BufStream::append(Buffer src) {
			Nat copy = src.filled();
			if (data.filled() + copy >= data.count())
				realloc(copy);

			Nat start = data.filled();
			memcpy(data.dataPtr() + start, src.dataPtr(), copy);
			data.filled(start + copy);
		}

		void BufStream::realloc(Nat count) {
			Nat currentCap = data.filled() - pos;
			Nat newCap = currentCap + count;

			// Is it possible to just discard data and fit the new data?
			if (newCap <= data.count()) {
				memmove(data.dataPtr(), data.dataPtr() + pos, currentCap);
			} else {
				// No, we need to allocate a new array.
				Buffer newData = buffer(engine(), max(newCap, 2 * data.count()));
				memcpy(newData.dataPtr(), data.dataPtr() + pos, currentCap);
				data = newData;
			}

			data.filled(currentCap);
			pos = 0;
		}

	}
}
