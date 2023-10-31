#pragma once
#include "Core/GcArray.h"
#include "Core/EnginePtr.h"

namespace storm {
	STORM_PKG(core.io);

	/**
	 * Buffer for use with streams. Designed to make it possible to allocate on the stack to avoid
	 * heap allocations (at least from C++).
	 *
	 * If multiple copies of a buffer are created, they all refer to the same backing storage,
	 * slightly breaking the memory model of Storm, as a buffer is a value that behaves like an
	 * object. This allows the `Buffer` to work properly in multithreaded operations with some care.
	 * Interfaces typically need to both accept and return a buffer to work properly with threads.
	 * If a thread-switch is not needed, then the same buffer can be returned which results in no
	 * copies being made. If a thread-switch is made, the buffer is copied, and the returned value
	 * with up-to-date information is used. See `core.io.IStream` for an example of this.
	 *
	 * When using the MPS: buffers are allocated in a pool that neither moves nor protects its
	 * contents. This pool have slightly worse performance compared to the default pools in Storm,
	 * but it is required when doing async IO, as we might otherwise confuse the GC.
	 */
	class Buffer {
		STORM_VALUE;
	public:
		// Default constructor. Create an empty buffer. Use the free function `buffer` to create a
		// buffer with a non-zero size.
		STORM_CTOR Buffer();

		// Get the number of bytes in the buffer in total.
		inline Nat STORM_FN count() const { return data ? (Nat)data->count : 0; }

		// Get how many bytes from the beginning are used in the buffer. The buffer itself generally
		// does not use the value. Rather, it is used as a universal marker for how much of the
		// buffer is used for actual data, and how much is free.
		inline Nat STORM_FN filled() const { return data ? Nat(data->filled) : 0; }

		// Set the number of bytes that are used. Limits the value to be maximum `count`. Marked as
		// an assign functions, so that `filled` can be used as a member variable in languages like
		// Basic Storm.
		inline void STORM_ASSIGN filled(Nat p) { if (data) data->filled = min(p, count()); }

		// Get the number of free bytes in the buffer, based on the `filled` member.
		inline Nat STORM_FN free() const { return count() - filled(); }

		// Put a single byte at the end of the buffer and increases `filled`. Returns `false` if
		// there is no room for the data.
		Bool STORM_FN push(Byte data);

		// Shift data `n` locations towards the front of the buffer. This effectively removes the
		// first `n` bytes in the buffer. Assumes that only data up until `filled` are relevant and
		// updates `filled` accordingly. Note that this requires copying bytes in the buffer.
		// Removing small amounts of data frequently is therefore expensive.
		void STORM_FN shift(Nat n);

		// Access individual bytes.
		Byte &STORM_FN operator [](Nat id) { return data->v[id]; }
		Byte operator [](Nat id) const { return data->v[id]; }

		// Deep copy.
		void STORM_FN deepCopy(CloneEnv *env);

		// Get a pointer to the data.
		byte *dataPtr() { return data ? data->v : null; }
		const byte *dataPtr() const { return data ? data->v : null; }

		// Is the buffer empty, according to the `filled` member?
		inline Bool STORM_FN empty() const { return filled() == 0; }

		// Are there any data in the buffer, according to the `filled` member?
		inline Bool STORM_FN any() const { return filled() > 0; }

		// Is the buffer full, according to the `filled` member?
		inline Bool STORM_FN full() const { return filled() == count(); }

	private:
		// Data.
		GcArray<Byte> *data;

		// From C++: create a buffer with a pre-allocated array.
		Buffer(GcArray<Byte> *data);

		friend Buffer buffer(EnginePtr e, Nat count);
		friend Buffer emptyBuffer(GcArray<Byte> *data);
		friend Buffer fullBuffer(GcArray<Byte> *data);
		friend Buffer buffer(EnginePtr e, const Byte *data, Nat count);
	};

	// Create a buffer with room for `count` bytes. It will be initially empty.
	Buffer STORM_FN buffer(EnginePtr e, Nat count);

	// Create a buffer using a (potentially stack-allocated) GcArray for backing data.
	Buffer emptyBuffer(GcArray<Byte> *data);
	Buffer fullBuffer(GcArray<Byte> *data);

	// Create a buffer by copying data from 'data'. Sets 'filled'.
	Buffer buffer(EnginePtr e, const Byte *data, Nat count);

	// Grow a buffer to the specified size. Copies the content and the `filled` member from `src`.
	Buffer STORM_FN grow(EnginePtr e, Buffer src, Nat newCount);

	// Analogous to the `cut` functions in `Str`. Extracts a range of bytes from a buffer.
	Buffer STORM_FN cut(EnginePtr e, Buffer src, Nat from);
	Buffer STORM_FN cut(EnginePtr e, Buffer src, Nat from, Nat to);

	// Output the buffer to a string buffer. Since a buffer is an unformatted sequence of bytes, it
	// is outputted as a hex dump. The location of filled is marked with a pipe (`|`). As such,
	// buffers are a convenient way to output data in hexadecimal form.
	//
	// The system provides an overload of `toS` for buffers, so it is possible to call
	// `Buffer().toS()`.
	StrBuf &STORM_FN operator <<(StrBuf &to, Buffer b);

	// Output the buffer, like `<<`, but including a second mark at the specified location. This
	// location is indicated with a `>` character.
	void STORM_FN outputMark(StrBuf &to, Buffer b, Nat markAt);

	// Conversion to/from UTF-8 strings. This is possible through the memory streams and text
	// interface as well, but this is more convenient in some cases.

	// Encode a string into UTF-8. It is also possible to use text streams for this, and they may be
	// more convenient in certain situations.
	Buffer STORM_FN toUtf8(Str *str);

	// Interpret a buffer as UTF-8 and convert it into a string. It is also possible to use text
	// streams for this, and they may be more convenient in certain situations.
	Str *STORM_FN fromUtf8(EnginePtr e, Buffer b);

}
