#pragma once
#include "Stream.h"

namespace storm {
	STORM_PKG(core.io);

	/**
	 * Helper class that provides limited buffering to support seeking without a native peek
	 * operation on the underlying stream.
	 */
	class PeekIStream : public IStream {
		STORM_CLASS;
	public:
		// Create.
		PeekIStream();

		// Copy.
		PeekIStream(const PeekIStream &o);

		// Are we at the end of the stream?
		virtual Bool STORM_FN more();

		// Read from the stream.
		using IStream::read;
		virtual Buffer STORM_FN read(Buffer to);

		// Peek data from the stream.
		using IStream::peek;
		virtual Buffer STORM_FN peek(Buffer to);

		// Close.
		virtual void STORM_FN close();

	protected:
		// Function which does the actual reading. We assume that end of stream is reached when this
		// returns zero.
		virtual Nat doRead(byte *to, Nat count);

	private:
		// Lookahead data (if any). Used when doing peek() operations.
		GcArray<byte> *lookahead;

		// How much of 'lookahead' have we consumed so far?
		Nat lookaheadStart;

		// Have wee seen the end of this stream?
		Bool atEof;

		// Try to fill the lookahead buffer with 'bytes' bytes. Returns the number of available
		// bytes in the lookahead buffer.
		Nat doLookahead(Nat bytes);

		// Get the number of bytes available in the lookahead (which are filled).
		Nat lookaheadAvail() const;

		// Ensure there is at least 'n' bytes in the lookahead buffer (which may or may not be filled).
		void ensureLookahead(Nat n);
	};

}
