#include "stdafx.h"
#include "Buffer.h"
#include "Core/CloneEnv.h"
#include "Core/Str.h"
#include "Core/StrBuf.h"
#include "Core/Convert.h"

namespace storm {

	Buffer::Buffer() : data(null) {}

	Buffer::Buffer(GcArray<Byte> *buf) : data(buf) {}

	void Buffer::shift(Nat n) {
		if (n >= filled()) {
			filled(0);
		} else {
			memmove(data->v, data->v + n, filled() - n);
			filled(filled() - n);
		}
	}

	Bool Buffer::push(Byte b) {
		if (!data)
			return false;
		if (data->filled >= data->count)
			return false;

		data->v[data->filled++] = b;
		return true;
	}

	void Buffer::deepCopy(CloneEnv *env) {
		if (data) {
			GcArray<Byte> *n = runtime::allocBuffer(env->engine(), data->count);
			n->filled = data->filled;
			for (nat i = 0; i < data->count; i++)
				n->v[i] = data->v[i];
			data = n;
		}
	}

	Buffer buffer(EnginePtr e, Nat count) {
		return Buffer(runtime::allocBuffer(e.v, count));
	}

	Buffer emptyBuffer(GcArray<Byte> *data) {
		Buffer r(data);
		r.filled(0);
		return r;
	}

	Buffer fullBuffer(GcArray<Byte> *data) {
		Buffer r(data);
		r.filled(Nat(data->count));
		return r;
	}

	Buffer buffer(EnginePtr e, const Byte *data, Nat count) {
		Buffer z(runtime::allocBuffer(e.v, count));
		memcpy(z.dataPtr(), data, count);
		z.filled(count);
		return z;
	}

	Buffer grow(EnginePtr e, Buffer src, Nat newCount) {
		Buffer r = buffer(e, newCount);
		memcpy(r.dataPtr(), src.dataPtr(), src.filled());
		r.filled(src.filled());
		return r;
	}

	Buffer cut(EnginePtr e, Buffer src, Nat from) {
		return cut(e, src, from, src.count());
	}

	Buffer cut(EnginePtr e, Buffer src, Nat from, Nat to) {
		if (to <= from)
			return buffer(e, 0);

		Buffer r = buffer(e, to - from);

		if (src.filled() > from) {
			Nat copy = min(to, src.filled()) - from;
			memcpy(r.dataPtr(), src.dataPtr() + from, copy);
			r.filled(copy);
		} else {
			r.filled(0);
		}

		return r;
	}

	StrBuf &operator <<(StrBuf &to, Buffer b) {
		outputMark(to, b, b.count() + 1);
		return to;
	}

	void outputMark(StrBuf &to, Buffer b, Nat mark) {
		const Nat width = 16;
		for (Nat i = 0; i <= b.count(); i++) {
			if (i % width == 0) {
				if (i > 0)
					to << S("\n");
				to << hex(i) << S(" ");
			}

			if (i == b.filled() && i == mark)
				to << S("|>");
			else if (i == b.filled())
				to << S("| ");
			else if (i == mark)
				to << S(" >");
			else
				to << S("  ");

			if (i < b.count())
				to << hex(b[i]);
		}
	}

	Buffer toUtf8(Str *str) {
		return fullBuffer((GcArray<Byte> *)toChar(str->engine(), str->c_str()));
	}

	Str *fromUtf8(EnginePtr e, Buffer buffer) {
		return new (e.v) Str(toWChar(e.v, (const char *)buffer.dataPtr(), buffer.filled()));
	}
}
