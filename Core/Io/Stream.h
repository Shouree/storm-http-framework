#pragma once
#include "Core/Object.h"
#include "Buffer.h"
#include "StreamError.h"
#include "Core/Exception.h"

namespace storm {
	STORM_PKG(core.io);

	/**
	 * Input and Output streams for Storm. These are abstract base classes
	 * and concrete implementations are provided for at least file system IO
	 * and standard input/output.
	 *
	 * When cloning streams, you get a new read-pointer to the underlying source.
	 * TODO: In some cases (such as networks) this does not makes sense...
	 */

	class RIStream;

	/**
	 * Input stream.
	 *
	 * An input stream reads data from some source. Input streams are generally not buffered, so it
	 * is usually a good idea to read data in chunks. This is done by many consumers of data from
	 * input streams, such as the text IO system.
	 */
	class IStream : public Object {
		STORM_CLASS;
	public:
		// Create.
		STORM_CTOR IStream();

		// Returns `false` if the end of the stream has been reached, and `true` otherwise. In some
		// cases it is not possible to determine the end of stream ahead of time. In such cases,
		// `more` might return `true` even if `read` will later report that it managed to read zero
		// bytes. Whenever `read` succeeds with zero bytes, `more` will start returning `false`
		// afterwards, except for streams that support timeouts. A timed-out read does not count
		// as an end of stream.
		virtual Bool STORM_FN more();

		// Read data from the stream and attempt to fill the bytes from `filled` to `count` in the
		// buffer `to` with bytes from the stream. Returns a buffer with the newly acquired data,
		// and a `filled` member that has been updated to reflect the number of read bytes.
		// Generally, the returned buffer refers to the same contents as `to` to avoid copying
		// potentially large amounts of data. The implementation is, however, free to re-allocate
		// the buffer as needed. In particular, this is always the case when the system needs to
		// execute code on another thread, and thereby copy the buffers. Because of this, users of
		// the `read` function shall always use the returned buffer and not rely on the buffer
		// passed as `to` being updated.
		//
		// The function blocks until at least one byte has been successfully read from the stream,
		// or if the end of stream has been reached. This means that if `filled` has not been
		// updated from its original location, then it is guaranteed that the end of the stream has
		// been reached. Note, however, that the `read` function does not guarantee that the entire
		// buffer has been filled. This often happens in the case of networking, for example. If the
		// remote end has only sent one byte, a read operation of 10 bytes will typically only
		// result in 1 out of 10 bytes being returned, since filling the buffer would potentially
		// block foreveer (for example, the remote end might wait for a response).
		virtual Buffer STORM_FN read(Buffer to);

		// Like `read(Buffer)`, but allocates a new buffer rather than reusing an existing one.
		Buffer STORM_FN read(Nat maxBytes);

		// Like `read`, but repeats the read operation until the buffer is full, or until the end of
		// the stream has been reached. This means that if the buffer is not full after `read` has
		// been called, then the end of stream has always been reached. Do, however, note that this
		// behavior may not always be desirable when working with pipes or networking.
		Buffer STORM_FN fill(Buffer to);

		// Like `fill(Buffer)`, but allocates a new buffer rather than reusing an existing one.
		Buffer STORM_FN fill(Nat bytes);

		// Peek bytes from the stream and attempt to fill `to` similarly to `read`. The difference
		// is that the bytes returned will not be consumed, and can be read again using a `read` or
		// another `peek` operation.

		// The exact number of bytes that can be safely acquired through a `peek` operation depends
		// on the stream in use.
		virtual Buffer STORM_FN peek(Buffer to);

		// Like `peek(Buffer)`, but allocates a new buffer rather than reusing an existing one.
		Buffer STORM_FN peek(Nat maxBytes);

		// Create a random access stream based on this stream. For streams that are random access,
		// this function simply returns the same stream, but casted to a random access stream. For
		// streams that do not natively support random access, it instead a `core.io.LazyMemIStream`
		// that buffers data to allow seeking from this point onwards. Regardless of which case was
		// used, the original stream does not need to be closed.
		virtual RIStream *STORM_FN randomAccess();

		// Closes the stream and frees any associated resources. As with `flush`, this is generally
		// propagated in the case of chained streams.
		//
		// The destructor of the `OStream` calls close as a last resort. Since output streams are
		// garbage collected, it might be a while before the destructor is called. As such, manually
		// closing streams is generally preferred.
		virtual void STORM_FN close();

		// Check if any error has occurred while reading. When an error has occurred, the stream
		// will act as if it has reached the end of file. The reason can be checked by calling this
		// function.
		virtual sys::ErrorCode STORM_FN error() const;

		// Read various primitive types from the stream. Throws an exception on error.
		// Call Int:read() etc. from Storm to access these members!
		Bool readBool();
		Byte readByte();
		Int readInt();
		Nat readNat();
		Long readLong();
		Word readWord();
		Float readFloat();
		Double readDouble();
	};


	/**
	 * Random access input stream.
	 *
	 * Random access streams inherit from the `IStream` class, and thus share the same basic
	 * interface for reading data.
	 */
	class RIStream : public IStream {
		STORM_CLASS;
	public:
		// Create.
		STORM_CTOR RIStream();

		// This implementation returns the same object.
		virtual RIStream *STORM_FN randomAccess();

		/**
		 * Extended interface.
		 */

		// Seek relative the start.
		virtual void STORM_FN seek(Word to);

		// Get current position.
		virtual Word STORM_FN tell();

		// Get the length of the input data, in bytes.
		virtual Word STORM_FN length();
	};


	/**
	 * An output stream.
	 *
	 * Output streams accept data and writes it to some destination. An output stream is implemented
	 * as subclasses to the `OStream` class. Output streams are not buffered in general, so it is
	 * usually a good idea to write data in chunks when possible. The class
	 * `core.io.BufferedOStream` can be used to wrap an output to ensure data is buffered.
	 */
	class OStream : public Object {
		STORM_CLASS;
	public:
		// Create.
		STORM_CTOR OStream();

		// Write all data from the start of the buffer and up to, but not including the `filled`
		// marker in the buffer. Blocks until all data is written, unless an error occurs. Returns
		// the number of bytes from `buf` that were written to the stream. The returned value will
		// be equal to the number of bytes in the buffer unless an error occurred.
		Nat STORM_FN write(Buffer buf);

		// Write all data from `start` up to, but not including the `filled` marker in the buffer.
		// Blocks until all data is written, unless an error occurs. Returns the number of bytes
		// from `buf` that were written to the stream. The returned value will be equal to the
		// number of bytes in the buffer unless an error occurred.
		virtual Nat STORM_FN write(Buffer buf, Nat start);

		// Flush any buffered data to the destination. The exact behavior of this operation depends
		// on the stream that is used. For example, file- and network streams are generally
		// unbuffered by default, so it is not necessary to flush them. There are, however, streams
		// that buffer the input for efficiency (e.g. `core.io.BufferedOStream`). These use
		// `flush()` to allow manually flushing the buffer. Calls to `flush` are generally passed
		// along chains of output streams. For example, calling `flush` on a `BufferedOStream` will
		// flush the `BufferedOStream` and call flush on whichever stream the `BufferedOStream` is
		// writing its data to. Returns `false` if the operation fails.
		virtual Bool STORM_FN flush();

		// Closes the stream and frees any associated resources. As with `flush`, this is generally
		// propagated in the case of chained streams.
		//
		// The destructor of the `OStream` calls close as a last resort. Since output streams are
		// garbage collected, it might be a while before the destructor is called. As such, manually
		// closing streams is generally preferred.
		virtual void STORM_FN close();

		// Check if any error has occurred during write. Errors generally cause `write` operations
		// to fail, but no exceptions are thrown. This makes it possible to check for errors at the
		// end of a large write.
		virtual sys::ErrorCode STORM_FN error() const;

		// Write various primitive types to the stream. Used to implement custom serialization.
		// Call Int:write etc. from storm to access these members!
		void writeBool(Bool v);
		void writeByte(Byte v);
		void writeInt(Int v);
		void writeNat(Nat v);
		void writeLong(Long v);
		void writeWord(Word v);
		void writeFloat(Float v);
		void writeDouble(Double v);

		// Overloaded version. Convenient from templates. (CL crashed when having pointer-to-member
		// as template parameter)
		void writeT(Bool v) { writeBool(v); }
		void writeT(Byte v) { writeByte(v); }
		void writeT(Int v) { writeInt(v); }
		void writeT(Nat v) { writeNat(v); }
		void writeT(Long v) { writeLong(v); }
		void writeT(Word v) { writeWord(v); }
		void writeT(Float v) { writeFloat(v); }
		void writeT(Double v) { writeDouble(v); }
	};

}
