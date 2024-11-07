#pragma once
#include "Socket.h"
#include "Core/Io/HandleStream.h"

namespace storm {
	STORM_PKG(core.net);

	class NetIStream;
	class NetOStream;

	/**
	 * Keepalive times for NetStream (TCP sockets).
	 *
	 * Interestingly enough, it is not always possible to get the current timeouts from a socket. As
	 * such, this class has three possible states:
	 *
	 * - Keepalive off.
	 * - Keepalive on, using system defaults.
	 * - Keepalive on, using custom times.
	 *
	 * The two times are `idle` and `interval`. The first (`idle`) specifies how long to wait before
	 * sending a keepalive packet after the connection has been idle. The second (`interval`)
	 * decides how often keepalive packets should be sent if a reply from the first keepalive is not
	 * received. Number of retries is not always configurable.
	 */
	class Keepalive {
		STORM_VALUE;
	public:
		// Default constructor, means "off".
		STORM_CTOR Keepalive() : enabled(false) {}

		// Create "off" explicitly.
		static Keepalive STORM_FN off() {
			return Keepalive();
		}

		// Create "on", but using defaults.
		static Keepalive STORM_FN on() {
			return Keepalive(Duration(), Duration());
		}

		// Create "on", specifying timeouts.
		static Keepalive STORM_FN on(Duration idle) {
			return Keepalive(idle, time::s(2));
		}
		static Keepalive STORM_FN on(Duration idle, Duration interval) {
			return Keepalive(idle, interval);
		}

		// Enable keepalive?
		Bool enabled;

		// Durations for idle and interval. If <= 0 is used, then the default time is used. Note
		// that both must be set at the same time.
		Duration idle;
		Duration interval;

		// Are the times default?
		Bool STORM_FN defaultTimes() const {
			return idle.v > 0 && interval.v > 0;
		}

		// To string.
		void STORM_FN toS(StrBuf *to) const;

	private:
		// Internal constructor.
		Keepalive(Duration idle, Duration interval)
			: enabled(true), idle(idle), interval(interval) {}
	};


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

		// Get the current keepalive setting.
		Keepalive STORM_FN keepalive() const;

		// Set the current keepalive setting.
		void STORM_ASSIGN keepalive(Keepalive k);
		void STORM_ASSIGN keepalive(Duration d) {
			keepalive(Keepalive::on(d));
		}

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

		// Current keepalive.
		Keepalive alive;
	};


	// Create a socket that is connected to a specific address.
	MAYBE(NetStream *) STORM_FN connect(Address *to);

	// Create a socket that is connected to a specific address, resolving the name first. If `host`
	// specifies a port, it overrides the port in `port`.
	MAYBE(NetStream *) STORM_FN connect(Str *host, Nat port);


	/**
	 * Input stream for the socket.
	 */
	class NetIStream : public HandleTimeoutIStream {
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
