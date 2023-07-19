#pragma once
#include "GcArray.h"

namespace storm {
	STORM_PKG(core);

	namespace pod {
		// Get the GcType for the type.
		template <class T>
		struct Gc {
			static const GcType type;
		};

		template <class T>
		struct Gc<T *> {
			static const GcType type;
		};


		template <class T>
		const GcType Gc<T>::type = {
			GcType::tArray,
			null,
			null,
			sizeof(T),
			0, {}
		};

		template <class T>
		const GcType Gc<T *>::type = {
			GcType::tArray,
			null,
			null,
			sizeof(T *),
			1, { 0 }
		};
	}

	/**
	 * Array that can store POD types without pointers or an array of pure pointers. This array
	 * starts preallocated with a number of elements so that no allocations have to be made until
	 * the preallocation is filled.
	 */
	template <class T, nat alloc>
	class PODArray {
	public:
		PODArray(Engine &e) : e(e) {
			data = pre;
			data->filled = 0;
		}

		// Add an element.
		void push(const T &e) {
			if (data->filled == data->count)
				grow();
			data->v[data->filled++] = e;
		}

		// Pop an element (does not clear storage).
		void pop() {
			if (data->filled > 0)
				data->filled--;
		}

		// Access elements.
		T &operator[] (nat id) {
			return data->v[id];
		}

		// Count.
		nat count() const {
			return Nat(data->filled);
		}

		// Clear (does not clear storage)
		void clear() {
			data->filled = 0;
		}

		// Reverse the data.
		void reverse() {
			nat first = 0;
			nat last = count();
			while ((first != last) && (first != --last)) {
				std::swap(data->v[first], data->v[last]);
				++first;
			}
		}

	private:
		PODArray(const PODArray &o);
		PODArray &operator =(const PODArray &o);

		// Engine.
		Engine &e;

		// Pre-allocated array.
		GcPreArray<T, alloc> pre;

		// Pointer to the current data.
		GcArray<T> *data;

		// Grow storage.
		void grow() {
			nat newCount = Nat(data->count) * 2;
			GcArray<T> *d = runtime::allocArray<T>(e, &pod::Gc<T>::type, newCount);
			d->filled = data->filled;
			memcpy(d->v, data->v, sizeof(T) * data->filled);
			data = d;
		}
	};

}
