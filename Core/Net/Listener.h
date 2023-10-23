#pragma once
#include "Socket.h"
#include "Address.h"
#include "NetStream.h"
#include "Core/EnginePtr.h"

namespace storm {
	STORM_PKG(core.net);

	/**
	 * A socket that listens for incoming TCP connections. Accepts connections on both IPv4 and IPv6
	 * protocols.
	 *
	 * Use `listen` to create listeners.
	 *
	 * By default, the SO_REUSEADDR option is set so that one can immediately re-use a port. This
	 * can be explicitly disabled by passing `false` to one of the `listen` functions.
	 */
	class Listener : public Socket {
		STORM_CLASS;
	public:
		// Create the listener from C++.
		Listener(os::Handle socket, os::Thread attachedTo);

		// Accept a new connection. Returns `null` if the listener has been closed.
		MAYBE(NetStream *) STORM_FN accept();

		// To string.
		virtual void STORM_FN toS(StrBuf *to) const;
	};

	// Listen on all interfaces on `port`. If `port` is zero, pick an unused port.
	MAYBE(Listener *) STORM_FN listen(EnginePtr e, Nat port);

	// Listen on all interfaces on `port`, explicitly specifying the use of SO_REUSEADDR.
	MAYBE(Listener *) STORM_FN listen(EnginePtr e, Nat port, Bool reuseAddr);

	// Listen on the address specified by `addr`.
	MAYBE(Listener *) STORM_FN listen(Address *addr);

	// Listen on the address specified by `addr`, explicitly specifying the use of SO_REUSEADDR.
	MAYBE(Listener *) STORM_FN listen(Address *addr, Bool reuseAddr);

}
