#pragma once
#include "Stream.h"
#include "PeekStream.h"
#include "OS/Sync.h"

namespace storm {
	STORM_PKG(core.io);

	/**
	 * Classes used for stdin and stdout streams, as these sometimes differs from regular file IO.
	 */

	enum StdStream {
		stdIn,
		stdOut,
		stdError,
	};


	/**
	 * Input stream from a standard input handle.
	 */
	class StdIStream : public PeekIStream {
		STORM_CLASS;
	public:
		// Create.
		StdIStream();

	protected:
		// Read.
		virtual PeekReadResult doRead(byte *to, Nat count);
	};


	/**
	 * Output stream to a standard output handle.
	 */
	class StdOStream : public OStream {
		STORM_CLASS;
	public:
		// Create.
		StdOStream(StdStream id);

		// Write data.
		using OStream::write;
		virtual void STORM_FN write(Buffer buf, Nat start);

	private:
		// Target stream.
		StdStream target;
	};


	/**
	 * Stack-allocated struct used to communicate requests to the broker.
	 */
	class StdRequest {
	public:
		StdRequest(StdStream stream, byte *to, Nat count);

		// Stream being interacted with.
		StdStream stream;

		// Buffer. 'count' is modified to reflect the number of bytes read.
		byte *buffer;
		Nat count;

		// Sema used to signal completion.
		os::Sema wait;

		// Pointer to the next request in line.
		StdRequest *next;
	};


	namespace proc {
		STORM_PKG(core.io.std);

		/**
		 * Get standard file handles.
		 */
		IStream *STORM_FN in(EnginePtr e);
		OStream *STORM_FN out(EnginePtr e);
		OStream *STORM_FN error(EnginePtr e);
	}


}
