#pragma once
#include "Core/Net/Address.h"

namespace sql {

	/**
	 * Describes how to connect to a database.
	 *
	 * Many databases support at least TCP connections and UNIX sockets/Win32 pipes. This class
	 * attempts to represent these choices in a generic fashion.
	 */
	class Host {
		STORM_VALUE;
	public:
		// Use a socket to connect to the specified address.
		static Host STORM_FN socket(Address *address);

		// Use a UNIX socket, or a pipe to connect. Specify the name/path.
		static Host STORM_FN local(Str *name);

		// Connect to localhost, using default method.
		static Host STORM_FN local();

		// Check if the connection is a socket.
		MAYBE(Address *) STORM_FN isSocket() const;

		// Check if the connection is a local pipe.
		MAYBE(Str *) STORM_FN isLocal() const;

	private:
		// Create a default initialized connection.
		Host();

		// Contains either:
		// - Address: connect to the specified host + port.
		// - Str    : connect to the specified socket.
		// - null   : connect to localhost.
		Object *data;
	};

	// Output.
	StrBuf *operator <<(StrBuf *to, const Host &c);

}
