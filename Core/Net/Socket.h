#pragma once
#include "Net.h"
#include "Address.h"
#include "Core/Timing.h"
#include "OS/Handle.h"

namespace storm {
	STORM_PKG(core.net);

	/**
	 * A generic socket.
	 *
	 * This socket encapsulates the generic parts of a socket, that is shared by TCP, UDP, and other
	 * protocols.
	 */
	class Socket : public Object {
		STORM_CLASS;
	public:
		// Create. Assumes the socket in 'handle' is set up for asynchronious operation.
		Socket(os::Handle handle, os::Thread attachedTo);

		// Copy ctor.
		Socket(const Socket &o);

		// Destroy.
		virtual ~Socket();

		// Close the socket. Calls 'close' on both the input and output streams.
		virtual void STORM_FN close();

		// To string.
		virtual void STORM_FN toS(StrBuf *to) const;

		// Get input buffer size in bytes.
		Nat STORM_FN inputBufferSize() const;

		// Set the input buffer size in bytes. The operating system may alter this value (eg. Linux doubles it).
		void STORM_ASSIGN inputBufferSize(Nat size);

		// Get the output buffer size in bytes.
		Nat STORM_FN outputBufferSize() const;

		// Set the output buffer size in bytes. The operating system may alter this value (eg. Linux doubles it).
		void STORM_ASSIGN outputBufferSize(Nat size);

	protected:
		// The handle for this socket.
		UNKNOWN(PTR_NOGC) os::Handle handle;

		// The thread the socket is associated with.
		UNKNOWN(PTR_NOGC) os::Thread attachedTo;
	};

}
