#pragma once
#include "Core/Object.h"
#include "Buffer.h"
#include "Core/Exception.h"

namespace storm {
	STORM_PKG(core.io);

	class EXCEPTION_EXPORT IoError : public Exception {
		STORM_EXCEPTION;
	public:
		IoError(const wchar *msg) {
			w = new (this) Str(msg);
			saveTrace();
		}
		STORM_CTOR IoError(Str *msg) {
			w = msg;
			saveTrace();
		}

		virtual void STORM_FN message(StrBuf *to) const {
			*to << w;
		}
	private:
		Str *w;
	};

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
	 */
	class IStream : public Object {
		STORM_CLASS;
	public:
		// Create.
		STORM_CTOR IStream();

		// Are we at the end of the stream?
		virtual Bool STORM_FN more();

		// Attempts to fill the remainder of the buffer by reading data from the stream. Starts
		// filling from the 'filled' parameter of the buffer. Returns the resulting buffer (which
		// might point to the original data), with a modified 'filled' parameter.
		// When the call returns, at least one byte has been read unless the end of stream was
		// reached. This means that the 'filled' parameter is always larger after the call
		// compared to before the call. This call does, however, not guarantee that the buffer
		// is completely full after the call, even if the end of stream was not reached.
		Buffer STORM_FN read(Nat maxBytes);
		virtual Buffer STORM_FN read(Buffer to);

		// Peek data. Semantics are the same as 'read', but bytes are not consumed. This means that
		// the next 'read' or 'peek' operation will see the same bytes the 'peek' operation saw.
		Buffer STORM_FN peek(Nat maxBytes);
		virtual Buffer STORM_FN peek(Buffer to);

		// Reads data until the buffer is filled or the end of stream is reached. In contrast to
		// 'read', the only situation where the buffer is not filled after the call returns is when
		// the end of stream is reached (or an error occurred).
		Buffer STORM_FN fill(Nat bytes);
		Buffer STORM_FN fill(Buffer to);

		// Get a random access IStream. May return the same stream!
		virtual RIStream *STORM_FN randomAccess();

		// Close this stream.
		virtual void STORM_FN close();

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

		// Get length.
		virtual Word STORM_FN length();
	};


	/**
	 * Output stream.
	 */
	class OStream : public Object {
		STORM_CLASS;
	public:
		// Create.
		STORM_CTOR OStream();

		// Write all data from the beginning of the buffer (or start, if specified) until the
		// buffer's 'filled' marker.
		void STORM_FN write(Buffer buf);
		virtual void STORM_FN write(Buffer buf, Nat start);

		// Flush this stream.
		virtual void STORM_FN flush();

		// Close.
		virtual void STORM_FN close();

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

		// Overloaded version. Convenient from templates. (CL crashed when having pointer-to-member as template parameter)
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
