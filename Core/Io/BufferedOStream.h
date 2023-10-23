#pragma once
#include "Stream.h"

namespace storm {
	STORM_PKG(core.io);

	/**
	 * Buffered output stream.
	 *
	 * Buffers up to `n` bytes before writing to the output stream. This is useful when writing to
	 * network streams and/or SSL streams, where there might be some overhead in writing many small
	 * chunks.
	 *
	 * The buffering means that it is important to call `flush` at appropriate times to synchronize
	 * communication.
	 */
	class BufferedOStream : public OStream {
		STORM_CLASS;
	public:
		// Create, specify buffer size.
		STORM_CTOR BufferedOStream(OStream *output, Nat bufferSize);

		// Create, use default size (4K).
		STORM_CTOR BufferedOStream(OStream *output);

		using OStream::write;
		// Write data.
		virtual void STORM_FN write(Buffer buf, Nat start);

		// Flush the stream.
		virtual void STORM_FN flush();

		// Close.
		virtual void STORM_FN close();

	private:
		// Underlying stream to output to.
		OStream *output;

		// Buffer. Allocated at startup.
		Buffer buffer;

		// Initialization logic.
		void init(Nat bufferSize);
	};

}
