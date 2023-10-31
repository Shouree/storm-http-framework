#pragma once
#include "Core/GcArray.h"
#include "Core/EnginePtr.h"

namespace sound {

	/**
	 * Buffer for use with sound streams. Designed to make it possible to allocate on the stack to
	 * avoid heap allocations (at least from C++).
	 *
	 * If multiple copies of a buffer are created, they all refer to the same backing storage,
	 * slightly breaking the memory model of Storm, as a buffer is a value that behaves like an
	 * object. This allows the `Buffer` to work properly in multithreaded operations with some care.
	 * Interfaces typically need to both accept and return a buffer to work properly with threads.
	 * If a thread-switch is not needed, then the same buffer can be returned which results in no
	 * copies being made. If a thread-switch is made, the buffer is copied, and the returned value
	 * with up-to-date information is used. See `core.io.IStream` for an example of this.
	 */
	class Buffer {
		STORM_VALUE;
	public:
		// Default constructor. Create an empty buffer. Use the function `buffer` to create a buffer
		// with non-zero size.
		STORM_CTOR Buffer();

		// Get the number of samples in the buffer in total.
		inline Nat STORM_FN count() const { return data ? Nat(data->count) : 0; }

		// Get the number of samples from the beginning of the buffer that are considered to contain
		// valid data. The buffer itself generally does not use this value. Rather, it is used as an
		// universal marker for how much of the buffer is used for actual data, and how much is
		// free.
		inline Nat STORM_FN filled() const { return data ? Nat(data->filled) : 0; }

		// Set the number of samples that are used. Limits the value to be maximum `count`. Marked
		// as an assign functions, so that `filled` can be used as a member variable in languages
		// like Basic Storm.
		inline void STORM_FN filled(Nat p) { if (data) data->filled = min(p, count()); }

		// Get the number of free samples in the buffer, based on the `filled` member.
		inline Nat STORM_FN free() const { return count() - filled(); }

		// Access individual samples.
		Float &STORM_FN operator [](Nat id) { return data->v[id]; }
		Float operator [](Nat id) const { return data->v[id]; }

		// Deep copy.
		void STORM_FN deepCopy(CloneEnv *env);

		// Get a pointer to the data.
		Float *dataPtr() { return data ? data->v : null; }

		// Is the buffer empty according to the `filled` member?
		inline Bool STORM_FN empty() const { return filled() == 0; }

		// Is there any data in the buffer, according to the `filled` member?
		inline Bool STORM_FN any() const { return filled() > 0; }

		// Is the buffer full, according to the `filled` member?
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

	// Create a buffer with room for `count` bytes. It will be empty initially.
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

	// Output the buffer, specifying an additional mark.
	void STORM_FN outputMark(StrBuf &to, Buffer b, Nat markAt);

	// Output the buffer. The system generates a `toS` function as well.
	StrBuf &STORM_FN operator <<(StrBuf &to, Buffer b);
}
