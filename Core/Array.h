#pragma once
#include "Object.h"
#include "Handle.h"
#include "GcArray.h"
#include "StrBuf.h"
#include "Fn.h"

namespace storm {
	STORM_PKG(core);

	/**
	 * Base class for arrays. Implements a slightly inconvenient interface that is reusable
	 * regardless of the contained type. Use the derived class Array<> for C++.
	 */
	class ArrayBase : public Object {
		STORM_CLASS;
	public:
		// Empty array.
		ArrayBase(const Handle &type);

		// Create an array with 'n' copies of 'data'.
		ArrayBase(const Handle &type, Nat n, const void *data);

		// Copy another array.
		ArrayBase(const ArrayBase &other);

		// Deep copy.
		virtual void STORM_FN deepCopy(CloneEnv *env);

		// Get size.
		inline Nat STORM_FN count() const { return data ? Nat(data->filled) : 0; }

		// Reserve size.
		void STORM_FN reserve(Nat count);

		// Clear.
		void STORM_FN clear();

		// Any elements?
		inline Bool STORM_FN any() const { return count() > 0; }

		// Empty?
		inline Bool STORM_FN empty() const { return count() == 0; }

		// Remove an element.
		void STORM_FN remove(Nat id);

		// Remove the last element.
		void STORM_FN pop();

		// Insert an element, giving it the id 'id'. 'id' <= 'count()'.
		void CODECALL insertRaw(Nat id, const void *item);

		// Append the entire contents of another array.
		ArrayBase *CODECALL appendRaw(ArrayBase *from);

		// Reverse the array.
		void STORM_FN reverse();

		// Get a random element. Throws if array is empty.
		void *CODECALL randomRaw() const;

		// Sort the array. Assumes the handle contains a '<' function.
		void CODECALL sortRaw();

		// Sort using a predicate.
		void CODECALL sortRawPred(FnBase *compare);

		// Remove duplicates, assumes sorted beforehand.
		void CODECALL removeDuplicatesRaw();

		// Remove duplicates, return the new version.
		ArrayBase *CODECALL withoutDuplicatesRaw();

		// To string.
		virtual void STORM_FN toS(StrBuf *to) const;

		// Handle of the contained type.
		const Handle &handle;

		// First element access.
		inline void *CODECALL firstRaw() const {
			return getRaw(0);
		}

		// Last element access.
		inline void *CODECALL lastRaw() const {
			return getRaw(count() - 1);
		}

		// Raw element access.
		inline void *CODECALL getRaw(Nat id) const {
			// We're calling a function to throw the exception to make it more likely that the
			// compiler inlines this function. Throwing an exception makes it far too large for
			// inlining.
			if (id < count()) {
				return ptr(id);
			} else {
				outOfBounds(id);
				return null;
			}
		}

		// Push an element.
		void CODECALL pushRaw(const void *element);


		/**
		 * Base class for the iterator.
		 * TODO: How does this fit with deepCopy?
		 */
		class Iter {
			STORM_VALUE;
		public:
			// Pointing to the end.
			Iter();

			// Pointing to a specific element in 'owner'.
			Iter(ArrayBase *owner);
			Iter(ArrayBase *owner, Nat index);

			// Compare.
			bool operator ==(const Iter &o) const;
			bool operator !=(const Iter &o) const;

			// Increase.
			Iter &operator ++();
			Iter operator ++(int z);

			// Raw get function.
			void *CODECALL getRaw() const;

			// Get the current index.
			inline Nat getIndex() const { return index; }

			// Raw pre- and post increment.
			Iter &CODECALL preIncRaw();
			Iter CODECALL postIncRaw();

		private:
			// Array we're pointing to.
			ArrayBase *owner;

			// Index.
			Nat index;

			// At end?
			bool atEnd() const;
		};

		// Raw iterator access.
		Iter CODECALL beginRaw();
		Iter CODECALL endRaw();

	protected:
		// Array contents (of bytes, to make addressing easier).
		GcArray<byte> *data;

		// Get the pointer to an element.
		inline void *ptr(GcArray<byte> *data, Nat id) const { return data->v + (id * handle.size); }
		inline void *ptr(Nat id) const { return ptr(data, id); }

		// Ensure 'data' can hold at least 'n' objects.
		void ensure(Nat n);

		// Throw out of bounds exception.
		void outOfBounds(Nat n) const;
	};

	// Declare the array's template in Storm.
	STORM_TEMPLATE(Array, createArray);

	/**
	 * Class used from C++.
	 */
	template <class T>
	class Array : public ArrayBase {
		STORM_SPECIAL;
	public:
		// Get the Storm type for this object.
		static Type *stormType(Engine &e) {
			return runtime::cppTemplate(e, ArrayId, 1, StormInfo<T>::id());
		}

		// Empty array.
		Array() : ArrayBase(StormInfo<T>::handle(engine())) {
			runtime::setVTable(this);
		}

		// 'n' elements.
		Array(Nat n, const T &item = T()) : ArrayBase(StormInfo<T>::handle(engine()), n, &item) {
			runtime::setVTable(this);
		}

		// Copy array.
		Array(const Array<T> &o) : ArrayBase(o) {
			runtime::setVTable(this);
		}

		// Create an array with one element in it.
		Array(const T &item) : ArrayBase(StormInfo<T>::handle(engine())) {
			runtime::setVTable(this);
			push(item);
		}

		// Element access.
		T &at(Nat i) {
			return *(T *)getRaw(i);
		}

		const T &at(Nat i) const {
			return *(const T *)getRaw(i);
		}

		// Get the last element (if any).
		T &last() {
			return *(T *)lastRaw();
		}

		const T &first() const {
			return *(const T *)firstRaw();
		}

		const T &last() const {
			return *(const T *)lastRaw();
		}

		// Insert an element.
		void push(const T &item) {
			pushRaw(&item);
		}

		// Insert at a specific location.
		void insert(Nat pos, const T &item) {
			insertRaw(pos, &item);
		}

		// Append another array.
		Array<T> *append(Array<T> *from) {
			appendRaw(from);
			return this;
		}

		// Random element.
		const T &random() const {
			return *(const T *)randomRaw();
		}

		// Sort. Assumes we have a '<' comparison in the handle.
		void sort() {
			sortRaw();
		}

		Array<T> *sorted() const {
			Array<T> *copy = new (this) Array<T>(*this);
			copy->sortRaw();
			return copy;
		}

		// Sort with custom predicate.
		void sort(Fn<Bool, T, T> *compare) {
			sortRawPred(compare);
		}

		Array<T> *sorted(Fn<Bool, T, T> *compare) {
			Array<T> *copy = new (this) Array<T>(*this);
			copy->sortRawPred(compare);
			return copy;
		}

		// Remove duplicates, assuming sorted beforehand.
		// Assumes we have a '==' or '<' in the handle.
		void removeDuplicates() {
			removeDuplicatesRaw();
		}

		// Removed duplicates.
		Array<T> *withoutDuplicates() {
			return (Array<T> *)withoutDuplicatesRaw();
		}

		// Append elements.
		Array<T> &operator <<(const T &item) {
			push(item);
			return *this;
		}

		/**
		 * Iterator.
		 */
		class Iter : public ArrayBase::Iter {
		public:
			Iter() : ArrayBase::Iter() {}

			Iter(Array<T> *owner, Nat index = 0) : ArrayBase::Iter(owner, index) {}

			T &operator *() const {
				return *(T *)getRaw();
			}

			T &v() const {
				return *(T *)getRaw();
			}

			T *operator ->() const {
				return (T *)getRaw();
			}

		};

		// Create iterators.
		Iter begin() {
			return Iter(this, 0);
		}

		Iter end() {
			return Iter(this, count());
		}
	};

}
