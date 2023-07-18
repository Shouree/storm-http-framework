#pragma once
#include "Core/GcArray.h"
#include "Core/EnginePtr.h"

namespace sound {

	/**
	 * Buffer for use with sound streams. Designed to make it possible to allocate on the stack to
	 * avoid heap allocations (at least from C++).
	 *
	 * If multiple copies of a buffer are created, they all refer to the same backing storage,
	 * slightly breaking the memory model of Storm.
	 *
	 * TODO: Make proper element access operators from Storm.
	 */
	class Buffer {
		STORM_VALUE;
	public:
		// Default constructor. Empty buffer.
		STORM_CTOR Buffer();

		// Number of bytes in total.
		inline Nat STORM_FN count() const { return data ? Nat(data->count) : 0; }

		// Number of bytes containing valid data. Constrains 'filled' to not be larger than 'count'.
		inline Nat STORM_FN filled() const { return data ? Nat(data->filled) : 0; }
		inline void STORM_FN filled(Nat p) { if (data) data->filled = min(p, count()); }

		// Element access.
		Float &STORM_FN operator [](Nat id) { return data->v[id]; }
		Float operator [](Nat id) const { return data->v[id]; }

		// Deep copy.
		void STORM_FN deepCopy(CloneEnv *env);

		// Get a pointer to the data.
		Float *dataPtr() { return data ? data->v : null; }

		// Empty/full?
		inline Bool STORM_FN empty() const { return filled() == 0; }
		inline Bool STORM_FN full() const { return filled() == count(); }

	private:
		// Data.
		GcArray<Float> *data;

		// From C++: create a buffer with a pre-allocated array.
		Buffer(GcArray<Float> *data);

		friend Buffer buffer(EnginePtr e, Nat count);
		friend Buffer emptyBuffer(GcArray<Float> *data);
		friend Buffer fullBuffer(GcArray<Float> *data);
		friend Buffer buffer(EnginePtr e, const Float *data, Nat count);
	};

	// Create a buffer.
	Buffer STORM_FN buffer(EnginePtr e, Nat count);

	// Create a buffer using a (potentially stack-allocated) GcArray for backing data. This function
	// assumes that the backing array contains data, so it sets 'filled' to the size of the
	// data. Use the constructor if this is not desired.
	Buffer emptyBuffer(GcArray<Float> *data);
	Buffer fullBuffer(GcArray<Float> *data);

	// Create a buffer by copying data from 'data'. Sets 'filled'.
	Buffer buffer(EnginePtr e, const Float *data, Nat count);

	// Grow a buffer.
	Buffer STORM_FN grow(EnginePtr e, Buffer src, Nat newCount);

	// Cut buffers into smaller pieces.
	Buffer cut(EnginePtr e, Buffer src, Nat from);
	Buffer cut(EnginePtr e, Buffer src, Nat from, Nat count);

	// Output.
	void STORM_FN outputMark(StrBuf &to, Buffer b, Nat markAt);
	StrBuf &STORM_FN operator <<(StrBuf &to, Buffer b);
}
