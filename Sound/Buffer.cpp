#include "stdafx.h"
#include "Buffer.h"
#include "Core/CloneEnv.h"

namespace sound {

	static const GcType bufType = {
		GcType::tArray,
		null,
		null,
		sizeof(Float),
		0,
		{}
	};

	Buffer::Buffer() : data(null) {}

	Buffer::Buffer(GcArray<Float> *buf) : data(buf) {}

	void Buffer::deepCopy(CloneEnv *env) {
		if (data) {
			GcArray<Float> *n = runtime::allocArray<Float>(env->engine(), &bufType, data->count);
			n->filled = data->filled;
			for (nat i = 0; i < data->count; i++)
				n->v[i] = data->v[i];
			data = n;
		}
	}

	Buffer buffer(EnginePtr e, Nat count) {
		return Buffer(runtime::allocArray<Float>(e.v, &bufType, count));
	}

	Buffer emptyBuffer(GcArray<Float> *data) {
		Buffer r(data);
		r.filled(0);
		return r;
	}

	Buffer fullBuffer(GcArray<Float> *data) {
		Buffer r(data);
		r.filled(Nat(data->count));
		return r;
	}

	Buffer buffer(EnginePtr e, const Float *data, Nat count) {
		Buffer z(runtime::allocArray<Float>(e.v, &bufType, count));
		memcpy(z.dataPtr(), data, count*sizeof(Float));
		z.filled(count);
		return z;
	}

	Buffer grow(EnginePtr e, Buffer src, Nat newCount) {
		Buffer r = sound::buffer(e, newCount);
		memcpy(r.dataPtr(), src.dataPtr(), src.filled()*sizeof(Float));
		r.filled(src.filled());
		return r;
	}

	Buffer cut(EnginePtr e, Buffer src, Nat from) {
		Nat count = 0;
		if (src.count() > from)
			count = src.count() - from;
		return cut(e, src, from, count);
	}

	Buffer cut(EnginePtr e, Buffer src, Nat from, Nat count) {
		Buffer r = sound::buffer(e, count);

		if (src.filled() > from) {
			Nat copy = src.filled() - from;
			memcpy(r.dataPtr(), src.dataPtr() + from, copy*sizeof(Float));
			r.filled(copy);
		} else {
			r.filled(0);
		}

		return r;
	}

	StrBuf &operator <<(StrBuf &to, Buffer b) {
		outputMark(to, b, b.count());
		return to;
	}

	void outputMark(StrBuf &to, Buffer b, Nat mark) {
		const Nat width = 16;
		for (Nat i = 0; i <= b.count(); i++) {
			if (i % width == 0) {
				if (i > 0)
					to << L"\n";
				to << hex(i) << L":";
			}

			if (i == b.filled() && i == mark)
				to << L"|>";
			else if (i == b.filled())
				to << L"| ";
			else if (i == mark)
				to << L" >";
			else
				to << L"  ";

			// TODO: Better formatting!
			if (i < b.count())
				to << b[i];
		}
	}


}
