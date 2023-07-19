#include "stdafx.h"
#include "ValueArray.h"
#include "Type.h"
#include "Core/StrBuf.h"

namespace storm {

	ValueArray::ValueArray() : data(null), arrayType(Value::stormType(engine())->gcArrayType()) {}

	ValueArray::ValueArray(const ValueArray &o) : data(null), arrayType(Value::stormType(engine())->gcArrayType()) {
		ensure(o.count());
		for (nat i = 0; i < o.count(); i++) {
			data->v[i] = o.data->v[i];
		}
	}

	void ValueArray::deepCopy(CloneEnv *env) {
		for (nat i = 0; i < count(); i++) {
			data->v[i].deepCopy(env);
		}
	}

	void ValueArray::clear() {
		data = null;
	}

	void ValueArray::toS(StrBuf *buf) const {
		*buf << L"[";

		if (count() > 0)
			*buf << data->v[0];

		for (nat i = 1; i < count(); i++)
			*buf << L", " << data->v[0];

		*buf << L"]";
	}

	void ValueArray::push(Value v) {
		nat c = count();
		ensure(c + 1);
		data->v[c] = v;
		data->filled = c + 1;
	}

	void ValueArray::ensure(Nat n) {
		if (n == 0)
			return;

		Nat oldCap = data ? Nat(data->count) : 0;
		if (oldCap >= n)
			return;

		// We need to grow 'data'. How much?
		Nat newCap = max(Nat(4), oldCap * 2);
		if (newCap < n)
			newCap = n;
		GcArray<Value> *newData = runtime::allocArray<Value>(engine(), arrayType, newCap);

		// Move data.
		if (data) {
			memcpy(&newData->v[0], &data->v[0], sizeof(Value)*oldCap);
			data->filled = 0;
			newData->filled = oldCap;
		}

		// Swap contents.
		data = newData;
	}

	Array<Value> *ValueArray::toArray() const {
		Array<Value> *r = new (this) Array<Value>();
		r->reserve(count());

		for (nat i = 0; i < count(); i++)
			r->push(data->v[i]);

		return r;
	}

}
