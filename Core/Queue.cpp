#include "stdafx.h"
#include "Queue.h"

namespace storm {

	QueueBase::QueueBase(const Handle &type) : handle(type), data(null), head(0) {}

	QueueBase::QueueBase(const QueueBase &other) : handle(other.handle), data(null), head(0) {
		if (other.empty())
			return;

		Nat count = other.count();
		ensure(count);

		Nat at = other.head;
		for (Nat i = 0; i < count; i++, step(at)) {
			handle.safeCopy(ptr(i), other.ptr(at));
		}
		data->filled = count;
	}

	void QueueBase::deepCopy(CloneEnv *env) {
		if (handle.deepCopyFn && data) {
			Nat at = head;
			for (Nat i = 0; i < data->filled; i++, step(at)) {
				(*handle.deepCopyFn)(ptr(at), env);
			}
		}
	}

	void QueueBase::ensure(Nat n) {
		if (n == 0)
			return;

		Nat oldCap = data ? Nat(data->count) : 0;
		if (oldCap >= n)
			return;

		Nat oldCount = count();

		Nat newCap = max(Nat(16), oldCap * 2);
		if (newCap < n)
			newCap = n;
		GcArray<byte> *newData = runtime::allocArray<byte>(engine(), handle.gcArrayType, newCap);

		if (data) {
			// Copy from head to the end.
			Nat firstBatch = min(oldCount, Nat(data->count) - head);
			memcpy(ptr(newData, 0), ptr(head), handle.size*firstBatch);

			// If needed, copy remaining elements.
			if (firstBatch < oldCount)
				memcpy(ptr(newData, firstBatch), ptr(0), handle.size*(oldCount - firstBatch));

			// Update metadata.
			data->filled = 0;
			newData->filled = oldCount;
		}

		// Swap contents.
		data = newData;
		head = 0;
	}

	void QueueBase::reserve(Nat n) {
		ensure(n);
	}

	void QueueBase::clear() {
		data = null;
		head = 0;
	}

	void *QueueBase::topRaw() {
		if (empty())
			throw new (this) QueueError(S("Cannot get the top element of an empty queue."));

		return ptr(head);
	}

	void QueueBase::pop() {
		if (empty())
			throw new (this) QueueError(S("Cannot pop an empty queue."));

		head++;
		if (head >= data->count)
			head -= Nat(data->count);
		data->filled--;
	}

	void QueueBase::pushRaw(const void *elem) {
		ensure(count() + 1);

		Nat to = head + Nat(data->filled);
		if (to >= data->count)
			to -= Nat(data->count);
		handle.safeCopy(ptr(to), elem);
		data->filled++;
	}

	void QueueBase::toS(StrBuf *to) const {
		*to << S("Queue:[");
		for (Nat at = head, i = 0; i < count(); i++, step(at)) {
			if (i > 0)
				*to << S(", ");
			(*handle.toSFn)(ptr(at), to);
		}
		*to << S("]");
	}

	QueueBase::Iter QueueBase::beginRaw() {
		return Iter(this, 0);
	}

	QueueBase::Iter QueueBase::endRaw() {
		return Iter(this, count());
	}

	QueueBase::Iter::Iter() : owner(null), index(0) {}

	QueueBase::Iter::Iter(QueueBase *owner, Nat index) : owner(owner), index(index) {}

	Bool QueueBase::Iter::operator ==(const Iter &o) const {
		if (atEnd() || o.atEnd())
			return atEnd() == o.atEnd();
		else
			return index == o.index && owner == o.owner;
	}

	Bool QueueBase::Iter::atEnd() const {
		return owner == null
			|| owner->count() <= index;
	}

	Bool QueueBase::Iter::operator !=(const Iter &o) const {
		return !(*this == o);
	}

	QueueBase::Iter &QueueBase::Iter::operator ++() {
		if (!atEnd())
			index++;
		return *this;
	}

	QueueBase::Iter QueueBase::Iter::operator ++(int) {
		Iter c = *this;
		++(*this);
		return c;
	}

	void *QueueBase::Iter::getRaw() const {
		if (atEnd()) {
			Engine &e = runtime::someEngine();
			throw new (e) QueueError(S("Trying to dereference an invalid iterator."));
		}
		Nat i = owner->head + index;
		if (i >= owner->data->count)
			i -= Nat(owner->data->count);
		return owner->ptr(i);
	}

	QueueBase::Iter &QueueBase::Iter::preIncRaw() {
		return operator ++();
	}

	QueueBase::Iter QueueBase::Iter::postIncRaw() {
		return operator ++(0);
	}

	QueueError::QueueError(const wchar *msg) : msg(new (engine()) Str(msg)) {
		saveTrace();
	}

	QueueError::QueueError(Str *msg) : msg(msg) {
		saveTrace();
	}

	void QueueError::message(StrBuf *to) const {
		*to << S("Queue error: ") << msg;
	}

}
