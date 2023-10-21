#pragma once
#include "Buffer.h"
#include "Stream.h"

namespace storm {
	STORM_PKG(core.io);

	/**
	 * A stream that lazily loads data from another stream. This is used to make other streams
	 * seekable. Note that 'length' will be updated throughout the lifetime of the stream.
	 */
	class LazyMemIStream : public RIStream {
		STORM_CLASS;
	public:
		// Create.
		STORM_CTOR LazyMemIStream(IStream *src);

		// Copy.
		LazyMemIStream(const LazyMemIStream &o);

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

		// Length. Will be updated as we read.
		virtual Word STORM_FN length();

		// Random access.
		virtual RIStream *STORM_FN randomAccess();

		// Output.
		void STORM_FN toS(StrBuf *to) const;

	private:
		// Source stream.
		IStream *src;

		// Data.
		Buffer data;

		// Position.
		Nat pos;

		// Extract more data from 'src'.
		void fill();
	};

}
