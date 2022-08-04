#pragma once
#include "GcType.h"

namespace storm {

	/**
	 * Helper struct to access gc:d arrays. The preprocessor recognices this class, so it is
	 * properly marked for the gc.
	 */
	template <class T>
	struct GcArray {
		// Number of elements. Set on allocation and then not allowed to change.
		const size_t count;

		// Number of used elements. Ignored by the GC and its usage is up to the user of the array.
		size_t filled;

		// Data.
		T v[1];
	};

	/**
	 * Variant used to pre-allocate a GcArray of a certain size somewhere.
	 */
	template <class T, size_t size>
	struct GcPreArray {
		const size_t count;
		size_t filled;
		T v[size];

		GcPreArray() : count(size), filled(0) {}
		// Specify a size that is possibly smaller thant the desired size.
		explicit GcPreArray(size_t count) : count(min(size, count)), filled(0) {}
		operator GcArray<T> *() const { return (GcArray<T> *)this; }
	};

	/**
	 * A GcArray of weak pointers.
	 */
	template <class T>
	struct GcWeakArray {
		// Number of elements. Tagged so the GC does not think it is a pointer.
		const size_t countI;

		// Number of splatted elements since reset. Tagged.
		size_t splattedI;

		// Data.
		T *v[1];

		// Number of elements.
		inline size_t count() const { return countI >> 1; }

		// Splatted elements.
		inline size_t splatted() const { return splattedI >> 1; }
		inline void splatted(size_t v) { splattedI = (v << 1) | 0x1; }
	};


	// Some common GcTypes for arrays, so we don't need to have multiple definitions of them hanging around.
	extern const GcType pointerArrayType;
	extern const GcType sizeArrayType; // size_t
	extern const GcType wordArrayType;
	extern const GcType natArrayType;
	extern const GcType wcharArrayType;
	extern const GcType byteArrayType;
}
