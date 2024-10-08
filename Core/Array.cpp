#include "stdafx.h"
#include "Array.h"
#include "StrBuf.h"
#include "GcType.h"
#include "Random.h"
#include "Sort.h"
#include "Exception.h"

namespace storm {


	ArrayBase::ArrayBase(const Handle &type) : handle(type), data(null) {}

	ArrayBase::ArrayBase(const Handle &type, Nat n, const void *src) : handle(type), data(null) {
		ensure(n);

		for (nat i = 0; i < n; i++) {
			handle.safeCopy(ptr(i), src);
			data->filled = i + 1;
		}
	}

	ArrayBase::ArrayBase(const ArrayBase &other) : handle(other.handle), data(null) {
		nat count = other.count();
		if (handle.copyFn) {
			ensure(count);

			for (nat i = 0; i < count; i++) {
				(*handle.copyFn)(ptr(i), other.ptr(i));
				// Remember the element was created, if we get an exception during copying.
				data->filled = i + 1;
			}
		} else if (count > 0) {
			// Memcpy will do!
			ensure(count);
			memcpy(ptr(0), other.ptr(0), count * handle.size);
			data->filled = count;
		}
	}

	void ArrayBase::deepCopy(CloneEnv *env) {
		if (handle.deepCopyFn) {
			for (nat i = 0; i < count(); i++)
				(*handle.deepCopyFn)(ptr(i), env);
		}
	}

	void ArrayBase::ensure(Nat n) {
		if (n == 0)
			return;

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
			memcpy(ptr(newData, 0), ptr(0), handle.size*oldCount);
			data->filled = 0;
			newData->filled = oldCount;
		}

		// Swap contents.
		data = newData;
	}

	void ArrayBase::reserve(Nat n) {
		ensure(n);
	}

	void ArrayBase::clear() {
		data = null;
	}

	void ArrayBase::remove(Nat id) {
		if (id >= count())
			throw new (this) ArrayError(id, count());

		handle.safeDestroy(ptr(id));
		memmove(ptr(id), ptr(id + 1), (count() - id - 1)*handle.size);
		data->filled--;
	}

	void ArrayBase::pop() {
		if (empty())
			throw new (this) ArrayError(0, 0, new (this) Str(S("pop")));

		Nat id = count() - 1;
		handle.safeDestroy(ptr(id));
		data->filled--;
	}

	void ArrayBase::insertRaw(Nat to, const void *item) {
		if (to > count())
			throw new (this) ArrayError(to, count(), new (this) Str(S("insert")));

		ensure(count() + 1);

		// Move the last few elements away.
		memmove(ptr(to + 1), ptr(to), (count() - to)*handle.size);
		// Insert the new one.
		handle.safeCopy(ptr(to), item);

		data->filled++;
	}

	ArrayBase *ArrayBase::appendRaw(ArrayBase *from) {
		Nat here = count();
		Nat copy = from->count();
		if (here + copy == 0)
			return this;

		ensure(here + copy);

		// Copy elements.
		if (handle.copyFn) {
			// Use the copy-ctor.
			for (Nat i = 0; i < copy; i++) {
				(*handle.copyFn)(ptr(here + i), ptr(from->data, i));
			}
		} else {
			// Memcpy is enough.
			memcpy(ptr(here), ptr(from->data, 0), handle.size * copy);
		}

		data->filled = here + copy;

		return this;
	}

	static void arraySwap(void *a, void *b, size_t size) {
		// Note: we can always move an object (the GC does it all the time), which is why this works
		// without issues. We just need to make sure we don't copy byte by byte, as the GC may pause
		// the world partly through and see an invalid pointer.

		nat copied = 0;
		{
			size_t *u = (size_t *)a;
			size_t *v = (size_t *)b;

			for (; copied + sizeof(size_t) <= size; copied += sizeof(size_t))
				std::swap(*u++, *v++);
		}

		// Any remaining bytes?
		{
			byte *u = (byte *)a;
			byte *v = (byte *)b;

			for (; copied < size; copied++)
				std::swap(u[copied], v[copied]);
		}
	}

	void ArrayBase::reverse() {
		if (empty())
			return;

		nat first = 0;
		nat last = count();

		while ((first != last) && (first != --last)) {
			arraySwap(ptr(first), ptr(last), handle.size);
			first++;
		}
	}

	void *ArrayBase::randomRaw() const {
		if (empty())
			throw new (this) ArrayError(0, 0, new (this) Str(S("random")));

		Nat id = rand(Nat(0), count());
		return getRaw(id);
	}

	void ArrayBase::sortRaw() {
		assert(handle.lessFn, L"The operator < is required when sorting an array.");

		if (empty())
			return;

		// We need one temporary element.
		ensure(count() + 1);

		SortData d(data, handle);
		sort(d);
	}

	void ArrayBase::sortRawPred(FnBase *compare) {
		if (empty())
			return;

		// We need one temporary element.
		ensure(count() + 1);

		SortData d(data, handle, compare);
		sort(d);
	}

	void ArrayBase::removeDuplicatesRaw() {
		if (count() == 0)
			return;

		Bool isEqual = true;
		Bool (*compare)(const void *, const void *) = handle.equalFn;
		if (!compare) {
			isEqual = false;
			compare = handle.lessFn;
		}

		Nat out = 0;
		for (Nat i = 1; i < count(); i++) {
			if (compare(ptr(out), ptr(i)) != isEqual) {
				++out;
				if (out != i)
					arraySwap(ptr(out), ptr(i), handle.size);
			}
		}

		while (count() > out + 1)
			pop();
	}

	void ArrayBase::removeDuplicatesRawPred(FnBase *compare) {
		if (count() == 0)
			return;

		RawFn compareFn = compare->rawCall();

		// Support *both* compare being < and == by checking:
		Bool isEqual = false;
		{
			const void *params[2] = { ptr(0), ptr(0) };
			compareFn.call(compare, &isEqual, params);
		}

		Nat out = 0;
		for (Nat i = 1; i < count(); i++) {
			// Compare:
			Bool equal = false;
			const void *params[2] = { ptr(out), ptr(i) };
			compareFn.call(compare, &equal, params);

			if (equal != isEqual) {
				++out;
				if (out != i)
					arraySwap(ptr(out), ptr(i), handle.size);
			}
		}

		while (count() > out + 1)
			pop();
	}

	ArrayBase *ArrayBase::withoutDuplicatesRaw() const {
		// Create a copy:
		void *copyMem = runtime::allocObject(sizeof(ArrayBase), runtime::typeOf(this));
		ArrayBase *copy = new (Place(copyMem)) ArrayBase(handle);
		runtime::setVTable(copy);

		// Copy unique elements:
		if (count() == 0)
			return copy;

		Bool isEqual = true;
		Bool (*compare)(const void *, const void *) = handle.equalFn;
		if (!compare) {
			isEqual = false;
			compare = handle.lessFn;
		}

		copy->pushRaw(ptr(0));
		for (Nat i = 1; i < count(); i++) {
			if (compare(copy->ptr(copy->count() - 1), ptr(i)) != isEqual)
				copy->pushRaw(ptr(i));
		}

		return copy;
	}

	ArrayBase *ArrayBase::withoutDuplicatesRawPred(FnBase *compare) const {
		// Create a copy:
		void *copyMem = runtime::allocObject(sizeof(ArrayBase), runtime::typeOf(this));
		ArrayBase *copy = new (Place(copyMem)) ArrayBase(handle);
		runtime::setVTable(copy);

		// Copy unique elements:
		if (count() == 0)
			return copy;

		RawFn compareFn = compare->rawCall();

		Bool isEqual = false;
		{
			const void *params[2] = { ptr(0), ptr(0) };
			compareFn.call(compare, &isEqual, params);
		}

		copy->pushRaw(ptr(0));
		for (Nat i = 1; i < count(); i++) {
			// Compare:
			Bool equal = false;
			const void *params[2] = { copy->ptr(copy->count() - 1), ptr(i) };
			compareFn.call(compare, &equal, params);

			if (equal != isEqual)
				copy->pushRaw(ptr(i));
		}

		return copy;
	}

	Nat ArrayBase::lowerBoundRaw(const void *find) const {
		Nat first = 0;
		Nat count = this->count();
		while (count > 0) {
			Nat step = count / 2;
			Nat middle = first + step;
			if ((*handle.lessFn)(ptr(middle), find)) {
				first = middle + 1;
				count -= step + 1;
			} else {
				count = step;
			}
		}
		return first;
	}

	Nat ArrayBase::lowerBoundRawPred(const void *find, FnBase *compare) const {
		RawFn compareFn = compare->rawCall();

		Nat first = 0;
		Nat count = this->count();
		while (count > 0) {
			Nat step = count / 2;
			Nat middle = first + step;

			Bool isLess = false;
			const void *params[2] = { ptr(middle), find };
			compareFn.call(compare, &isLess, params);

			if (isLess) {
				first = middle + 1;
				count -= step + 1;
			} else {
				count = step;
			}
		}
		return first;
	}

	Nat ArrayBase::upperBoundRaw(const void *find) const {
		Nat first = 0;
		Nat count = this->count();
		while (count > 0) {
			Nat step = count / 2;
			Nat middle = first + step;
			if (!(*handle.lessFn)(find, ptr(middle))) {
				first = middle + 1;
				count -= step + 1;
			} else {
				count = step;
			}
		}
		return first;
	}

	Nat ArrayBase::upperBoundRawPred(const void *find, FnBase *compare) const {
		RawFn compareFn = compare->rawCall();

		Nat first = 0;
		Nat count = this->count();
		while (count > 0) {
			Nat step = count / 2;
			Nat middle = first + step;

			Bool isLess = false;
			const void *params[2] = { find, ptr(middle) };
			compareFn.call(compare, &isLess, params);

			if (!isLess) {
				first = middle + 1;
				count -= step + 1;
			} else {
				count = step;
			}
		}
		return first;
	}

	Bool ArrayBase::equalRaw(const ArrayBase *other) const {
		if (count() != other->count())
			return false;

		for (Nat i = 0; i < count(); i++)
			if (!handle.equal(ptr(i), other->ptr(i)))
				return false;

		return true;
	}

	Bool ArrayBase::lessRaw(const ArrayBase *other) const {
		Nat smallest = min(count(), other->count());

		if (handle.equalFn) {
			for (Nat i = 0; i < smallest; i++) {
				if (!(*handle.equalFn)(ptr(i), other->ptr(i)))
					return (*handle.lessFn)(ptr(i), other->ptr(i));
			}
		} else {
			for (Nat i = 0; i < smallest; i++) {
				if ((*handle.lessFn)(ptr(i), other->ptr(i)))
					return true;
				if ((*handle.lessFn)(other->ptr(i), ptr(i)))
					return false;
			}
		}

		return count() < other->count();
	}

	void ArrayBase::toS(StrBuf *to) const {
		*to << S("[");
		if (count() > 0)
			(*handle.toSFn)(ptr(0), to);

		for (nat i = 1; i < count(); i++) {
			*to << S(", ");
			(*handle.toSFn)(ptr(i), to);
		}
		*to << S("]");
	}

	void ArrayBase::pushRaw(const void *element) {
		Nat c = count();
		ensure(c + 1);

		if (handle.copyFn) {
			(*handle.copyFn)(ptr(c), element);
		} else {
			memcpy(ptr(c), element, handle.size);
		}
		data->filled = c + 1;
	}

	void ArrayBase::outOfBounds(Nat n) const {
		throw new (this) ArrayError(n, count());
	}

	ArrayBase::Iter ArrayBase::beginRaw() {
		return Iter(this, 0);
	}

	ArrayBase::Iter ArrayBase::endRaw() {
		return Iter(this, count());
	}

	ArrayBase::Iter::Iter() : owner(0), index(0) {}

	ArrayBase::Iter::Iter(ArrayBase *owner) : owner(owner), index(0) {}

	ArrayBase::Iter::Iter(ArrayBase *owner, Nat index) : owner(owner), index(index) {}

	bool ArrayBase::Iter::operator ==(const Iter &o) const {
		if (atEnd() || o.atEnd())
			return atEnd() == o.atEnd();
		else
			return index == o.index && owner == o.owner;
	}

	bool ArrayBase::Iter::atEnd() const {
		return owner == null
			|| owner->count() <= index;
	}

	bool ArrayBase::Iter::operator !=(const Iter &o) const {
		return !(*this == o);
	}

	ArrayBase::Iter &ArrayBase::Iter::operator ++() {
		if (!atEnd())
			index++;
		return *this;
	}

	ArrayBase::Iter ArrayBase::Iter::operator ++(int) {
		Iter c = *this;
		++(*this);
		return c;
	}

	void *ArrayBase::Iter::getRaw() const {
		if (atEnd()) {
			Engine &e = runtime::someEngine();
			throw new (e) ArrayError(index, owner->count(), new (e) Str(S("iterator")));
		}
		return owner->getRaw(index);
	}

	ArrayBase::Iter &ArrayBase::Iter::preIncRaw() {
		return operator ++();
	}

	ArrayBase::Iter ArrayBase::Iter::postIncRaw() {
		return operator ++(0);
	}

}
