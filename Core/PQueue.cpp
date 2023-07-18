#include "stdafx.h"
#include "PQueue.h"
#include "Sort.h"

namespace storm {

	static GcArray<byte> *copyArray(const ArrayBase *src) {
		const Handle &h = src->handle;
		GcArray<byte> *r = runtime::allocArray<byte>(src->engine(), h.gcArrayType, src->count() + 1);

		for (Nat i = 0; i < src->count(); i++) {
			r->filled = i + 1;
			h.safeCopy(r->v + i*h.size, src->getRaw(i));
		}

		return r;
	}

	PQueueBase::PQueueBase(const Handle &type) : handle(type), data(null), compare(null) {}

	PQueueBase::PQueueBase(const ArrayBase *src) : handle(src->handle), compare(null) {
		data = copyArray(src);

		SortData d(data, handle);
		makeHeap(d);
	}

	PQueueBase::PQueueBase(const Handle &type, FnBase *compare) : handle(type), data(null), compare(compare) {}

	PQueueBase::PQueueBase(const ArrayBase *src, FnBase *compare) : handle(src->handle), compare(compare) {
		data = copyArray(src);

		SortData d(data, handle, compare);
		makeHeap(d);
	}

	PQueueBase::PQueueBase(const PQueueBase &o) : handle(o.handle), compare(o.compare) {
		ensure(o.count());

		for (Nat i = 0; i < o.count(); i++) {
			data->filled = i + 1;
			handle.safeCopy(data->v + i*handle.size, o.data->v + i*handle.size);
		}
	}

	void PQueueBase::deepCopy(CloneEnv *env) {
		if (handle.deepCopyFn) {
			for (Nat i = 0; i < count(); i++)
				(*handle.deepCopyFn)(data->v + i*handle.size, env);
		}

		if (compare)
			cloned(compare, env);
	}

	void PQueueBase::reserve(Nat count) {
		ensure(count);
	}

	void PQueueBase::pop() {
		SortData d(data, handle, compare);
		heapRemove(d);

		handle.safeDestroy(data->v + data->filled*handle.size);
		data->filled--;
	}

	void PQueueBase::pushRaw(const void *src) {
		ensure(count() + 1);

		SortData d(data, handle, compare);
		heapInsert(src, d);

		data->filled++;
	}

	void PQueueBase::throwError() const {
		throw new (this) PQueueError(S("Trying to access elements in an empty priority queue."));
	}

	void PQueueBase::ensure(Nat n) {
		if (n == 0)
			return;

		n++; // We need an additional element for swap space during heap operations.

		Nat oldCap = data ? Nat(data->count) : 0;
		if (oldCap >= n)
			return;

		Nat oldCount = count();

		// We need to grow 'data'. How much?
		Nat newCap = max(Nat(16), oldCap * 2);
		if (newCap < n)
			newCap = n;
		GcArray<byte> *newData = runtime::allocArray<byte>(engine(), handle.gcArrayType, newCap);

		// Move data.
		if (data) {
			memcpy(newData->v, data->v, handle.size*oldCount);
			data->filled = 0;
			newData->filled = oldCount;
		}

		// Swap contents.
		data = newData;
	}

	void PQueueBase::toS(StrBuf *to) const {
		*to << S("PQ:{");
		if (count() > 0)
			(*handle.toSFn)(data->v, to);

		for (nat i = 1; i < count(); i++) {
			*to << S(", ");
			(*handle.toSFn)(data->v + i*handle.size, to);
		}
		*to << S("}");
	}

}
