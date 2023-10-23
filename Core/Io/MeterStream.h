#pragma once
#include "Stream.h"

namespace storm {
	STORM_PKG(core.io);

	/**
	 * A stream that meters the number of bytes written to it, so that it is possible to store
	 * offsets to data structures in the file.
	 */
	class MeterOStream : public OStream {
		STORM_CLASS;
	public:
		// Create.
		STORM_CTOR MeterOStream(OStream *to);

		// Write.
		virtual void STORM_FN write(Buffer buf, Nat start);

		// Flush the stream.
		virtual void STORM_FN flush();

		// Close.
		virtual void STORM_FN close();

		// Get the current position.
		Word STORM_FN tell();

		// Reset the current position.
		void STORM_FN reset();

	private:
		// Output to.
		OStream *to;

		// Position.
		Word pos;
	};

	// TODO: Eventually, we might want a MeterIStream for completeness. However, that is not as
	// useful since we have RIStream, which provides proper seeking and telling.
}
