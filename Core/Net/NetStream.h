#pragma once
#include "Socket.h"
#include "Core/Io/HandleStream.h"

namespace storm {
	STORM_PKG(core.net);

	class NetIStream;
	class NetOStream;

	/**
	 * A TCP socket.
	 *
	 * The socket contain two streams: one for reading data and another for writing data. These
	 * streams may be used simultaneously. As for regular streams, it is not a good idea to share
	 * these streams between different threads, even if it is possible.
	 *
	 * Sockets are created either by calling the 'connect' function, or by a Listener object.
	 */
	class NetStream : public Socket {
		STORM_CLASS;
	public:
		// Create. Assumes the socket in 'handle' is set up for asynchronious operation.
		NetStream(os::Handle handle, os::Thread attachedTo, Address *peer);

		// Deep copy.
		virtual void STORM_FN deepCopy(CloneEnv *env);

		// Get the input stream.
		NetIStream *STORM_FN input() const;

		// Get the output stream.
		NetOStream *STORM_FN output() const;

		// Get the value of the `nodelay` socket option.
		Bool STORM_FN nodelay() const;

		// Set the `nodelay` socket option.
		void STORM_ASSIGN nodelay(Bool v);

		// Get the remote host we're connected to.
		Address *STORM_FN remote() const { return peer; }

		// To string.
		virtual void STORM_FN toS(StrBuf *to) const;

	protected:
		friend class NetIStream;
		friend class NetOStream;

		// Closed ends of the socket.
		Nat closed;

		enum {
			closeRead = 0x1,
			closeWrite = 0x2
		};

		// Close one end of the socket.
		void closeEnd(Nat which);

		// Input and output streams.
		NetIStream *i;
		NetOStream *o;

		// Connected peer (if any).
		Address *peer;
	};


	// Create a socket that is connected to a specific address.
	MAYBE(NetStream *) STORM_FN connect(Address *to);

	// Create a socket that is connected to a specific address, resolving the name first. If `host`
	// specifies a port, it overrides the port in `port`.
	MAYBE(NetStream *) STORM_FN connect(Str *host, Nat port);


	/**
	 * Input stream for the socket.
	 */
	class NetIStream : public HandleIStream {
		STORM_CLASS;
	public:
		// Not exposed to Storm. Created by the Socket.
		NetIStream(NetStream *owner, os::Thread attachedTo);

		// Destroy.
		virtual ~NetIStream();

		// Close this stream.
		virtual void STORM_FN close();

	private:
		// Owner.
		NetStream *owner;
	};


	/**
	 * Output stream for the socket.
	 */
	class NetOStream : public HandleOStream {
		STORM_CLASS;
	public:
		// Not exposed to Storm. Created by the socket.
		NetOStream(NetStream *owner, os::Thread attachedTo);

		// Destroy.
		virtual ~NetOStream();

		// Close.
		virtual void STORM_FN close();

	private:
		// Owner.
		NetStream *owner;
	};

}
