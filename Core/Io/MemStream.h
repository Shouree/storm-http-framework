#pragma once
#include "Buffer.h"
#include "Stream.h"

namespace storm {
	STORM_PKG(core.io);

	/**
	 * A stream that reads from a memory buffer.
	 */
	class MemIStream : public RIStream {
		STORM_CLASS;
	public:
		// Create from a buffer.
		STORM_CTOR MemIStream(Buffer b);

		// Copy.
		MemIStream(const MemIStream &o);

		// Deep copy.
		virtual void STORM_FN deepCopy(CloneEnv *env);

		// More?
		virtual Bool STORM_FN more();

		// Read.
		using RIStream::read;
		virtual Buffer STORM_FN read(Buffer to);

		// Peek.
		using RIStream::peek;
		virtual Buffer STORM_FN peek(Buffer to);

		// Seek.
		virtual void STORM_FN seek(Word to);

		// Tell.
		virtual Word STORM_FN tell();

		// Length.
		virtual Word STORM_FN length();

		// Random access.
		virtual RIStream *STORM_FN randomAccess();

		// Output.
		void STORM_FN toS(StrBuf *to) const;

	private:
		// Data.
		Buffer data;

		// Position.
		nat pos;
	};


	/**
	 * A stream that writes to a memory buffer.
	 */
	class MemOStream : public OStream {
		STORM_CLASS;
	public:
		// Create.
		STORM_CTOR MemOStream();

		// Create. Append to a buffer.
		STORM_CTOR MemOStream(Buffer appendTo);

		// Copy.
		MemOStream(const MemOStream &o);

		// Deep copy.
		void STORM_FN deepCopy(CloneEnv *env);

		// Write.
		using OStream::write;
		virtual Nat STORM_FN write(Buffer buf, Nat start);

		// Get the buffer.
		Buffer STORM_FN buffer();

		// Output.
		void STORM_FN toS(StrBuf *to) const;

	private:
		Buffer data;
	};

}
