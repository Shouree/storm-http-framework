#pragma once
#include "Fn.h"
#include "Array.h"
#include "GcArray.h"
#include "Exception.h"

namespace storm {
	STORM_PKG(core);

	/**
	 * Custom error type.
	 */
	class EXCEPTION_EXPORT PQueueError : public Exception {
		STORM_EXCEPTION;
	public:
		PQueueError(const wchar *msg) {
			this->msg = new (this) Str(msg);
			saveTrace();
		}
		STORM_CTOR PQueueError(Str *msg) {
			this->msg = msg;
			saveTrace();
		}
		virtual void STORM_FN message(StrBuf *to) const {
			*to << msg;
		}
	private:
		Str *msg;
	};

	/**
	 * Base class for priority queues. Implements a slightly inconvenient interface that is reusable
	 * regardless of the contained type. Use PQueue<T> for convenient usage from C++.
	 */
	class PQueueBase : public Object {
		STORM_CLASS;
	public:
		// Create an empty queue using '<'.
		PQueueBase(const Handle &type);
		PQueueBase(const ArrayBase *src);

		// Create an empty queue using 'compare'.
		PQueueBase(const Handle &type, FnBase *compare);
		PQueueBase(const ArrayBase *src, FnBase *compare);

		// Copy another queue.
		PQueueBase(const PQueueBase &o);

		// Deep copy.
		virtual void STORM_FN deepCopy(CloneEnv *env);

		// Type contained here.
		const Handle &handle;

		// Get size.
		virtual Nat STORM_FN count() const { return data ? Nat(data->filled) : 0; }

		// Reserve space for 'count' elements.
		virtual void STORM_FN reserve(Nat count);

		// Any elements?
		inline Bool STORM_FN any() const { return count() > 0; }

		// Empty?
		inline Bool STORM_FN empty() const { return count() == 0; }

		// Get the top element. Throws if empty.
		inline const void *topRaw() const {
			if (empty())
				throwError();
			// Top element is always in the first location.
			return data->v;
		}

		// Pop the top element. Throws if empty.
		void STORM_FN pop();

		// Push a new element.
		void pushRaw(const void *elem);

	protected:
		// Output.
		virtual void STORM_FN toS(StrBuf *to) const;

	private:
		// Data.
		GcArray<byte> *data;

		// Comparison function.
		FnBase *compare;

		// Ensure we have at least 'n' elements and an additional element for swap space.
		void ensure(Nat n);

		// Throw an 'empty stack' exception.
		void throwError() const;
	};

	// Declare the template in Storm.
	STORM_TEMPLATE(PQueue, createPQueue);

	/**
	 * PQueue class usable from C++.
	 */
	template <class T>
	class PQueue : public PQueueBase {
		STORM_SPECIAL;
	public:
		static Type *stormType(Engine &e) {
			return runtime::cppTemplate(e, PQueueId, 1, StormInfo<T>::id());
		}

		// Empty queue.
		PQueue() : PQueueBase(StormInfo<T>::handle(engine())) {
			runtime::setVTable(this);
		}

		// Create from array.
		PQueue(const Array<T> *src) : PQueueBase(src) {
			runtime::setVTable(this);
		}

		// Create with compare function.
		PQueue(Fn<Bool, T, T> *compare) : PQueueBase(StormInfo<T>::handle(engine()), compare) {
			runtime::setVTable(this);
		}

		// Create from array with compare function.
		PQueue(const Array<T> *src, Fn<Bool, T, T> *compare) : PQueueBase(src, compare) {
			runtime::setVTable(this);
		}

		// Get the topmost element.
		const T &top() const {
			return *(const T *)topRaw();
		}

		// Push an element.
		void push(const T &elem) {
			pushRaw(&elem);
		}

		PQueue &operator <<(const T &elem) {
			pushRaw(&elem);
			return *this;
		}

	};

}
