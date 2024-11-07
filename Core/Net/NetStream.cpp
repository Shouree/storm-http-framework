#include "stdafx.h"
#include "NetStream.h"

namespace storm {

	void Keepalive::toS(StrBuf *to) const {
		if (empty()) {
			*to << S("<no keepalive>");
		} else {
			*to << S("{ idle: ") << idle << S(", interval: ") << interval << S(" }");
		}
	}

	NetStream::NetStream(os::Handle handle, os::Thread attachedTo, Address *peer)
		: Socket(handle, attachedTo), closed(0), peer(peer) {

		i = new (this) NetIStream(this, attachedTo);
		o = new (this) NetOStream(this, attachedTo);
	}

	void NetStream::deepCopy(CloneEnv *env) {
		cloned(i, env);
		cloned(o, env);
	}

	NetIStream *NetStream::input() const {
		return i;
	}

	NetOStream *NetStream::output() const {
		return o;
	}

	Bool NetStream::nodelay() const {
		int v = 0;
		getSocketOpt(handle, IPPROTO_TCP, TCP_NODELAY, &v, sizeof(v));
		return v != 0;
	}

	void NetStream::nodelay(Bool v) {
		int val = v ? 1 : 0;
		setSocketOpt(handle, IPPROTO_TCP, TCP_NODELAY, &val, sizeof(val));
	}

#if defined(WINDOWS)

	Keepalive NetStream::keepalive() const {
		tcp_keepalive info = { 0, 0, 0 };
		DWORD bytesOut = 0;
		int ok = WSAIoctl(socket.v(), SIO_KEEPALIVE_VALS,
						NULL, NULL, // No input
						&info, sizeof(info), &bytesOut, // ...but output
						NULL, NULL); // No OVERLAPPED
		if (ok != 0 || bytesOut < sizeof(info)) {
			throw new (this) NetError(S("Failed to get keepalive information."));
		}

		if (info.onoff == 0)
			return Keepalive();
		else
			return Keepalive(time::ms(info.keepalivetime), time::ms(info.keepaliveinterval));
	}

	void NetStream::keepalive(Keepalive d) {
		Bool on = d.any();
		tcp_keepalive info = {
			on ? 1 : 0,
			// Defaults are the default on Linux. Only used whenever we have keepalive off.
			on ? d.idle.inMs() : 120 * 60 * 1000,
			on ? d.interval.inMs() : 75 * 1000,
		};
		int ok = WSAIoctl(socket.v(), SIO_KEEPALIVE_VALS,
						&info, sizeof(info), // input
						NULL, NULL, NULL, // no output
						NULL, NULL); // not OVERLAPPED
		if (ok != 0) {
			throw new (this) NetError(S("Failed to set keepalive information."));
		}
	}

#elif defined(LINUX)

	Keepalive NetStream::keepalive() const {
		int enabled = 0;
		if (!getSocketOpt(handle, SOL_SOCKET, SO_KEEPALIVE, &enabled, sizeof(enabled)))
			throw new (this) NetError(S("Failed to get SO_KEEPALIVE."));

		if (enabled == 0)
			return Keepalive();

		int idletime = 0;
		if (!getSocketOpt(handle, SOL_TCP, TCP_KEEPIDLE, &idletime, sizeof(idletime)))
			throw new (this) NetError(S("Failed to get TCP_KEEPIDLE."));
		int idleinterval = 0;
		if (!getSocketOpt(handle, SOL_TCP, TCP_KEEPINTVL, &idleinterval, sizeof(idleinterval)))
			throw new (this) NetError(S("Failed to get TCP_KEEPINTVL."));

		return Keepalive(time::s(idletime), time::s(idleinterval));
	}

	void NetStream::keepalive(Keepalive d) {
		int enabled = d.any() ? 1 : 0;
		if (!setSocketOpt(handle, SOL_SOCKET, SO_KEEPALIVE, &enabled, sizeof(enabled)))
			throw new (this) NetError(S("Failed to set SO_KEEPALIVE."));

		if (enabled) {
			int idletime = d.idle.inS();
			int idleinterval = d.interval.inS();

			if (!setSocketOpt(handle, SOL_TCP, TCP_KEEPIDLE, &idletime, sizeof(idletime)))
				throw new (this) NetError(S("Failed to set TCP_KEEPIDLE."));
			if (!setSocketOpt(handle, SOL_TCP, TCP_KEEPINTVL, &idleinterval, sizeof(idleinterval)))
				throw new (this) NetError(S("Failed to set TCP_KEEPINTVL."));
		}
	}

#else
#error "TODO: Implement keepalive for your OS."
#endif

	void NetStream::closeEnd(Nat which) {
		Nat old, w;
		do {
			old = atomicRead(closed);
			w = old | which;
		} while (atomicCAS(closed, old, w) != old);

		if (w == (closeRead | closeWrite) && handle) {
			closeSocket(handle, attachedTo);
			handle = os::Handle();
		}
	}

	void NetStream::toS(StrBuf *to) const {
		Socket::toS(to);
		if (handle)
			*to << S(" <-> ") << peer;
	}

	/**
	 * Connect.
	 */

	MAYBE(NetStream *) connect(Address *to) {
		initSockets();

		sockaddr_storage data;
		sockaddr *sa = (sockaddr *)&data;
		to->fill(sa);

		os::Handle h = createTcpSocket(sa->sa_family);
		os::Thread current = os::Thread::current();
		current.attach(h);

		try {
			// Connect!
			bool ok = connectSocket(h, current, sa, sizeof(data));
			if (!ok)
				return null;

			return new (to) NetStream(h, current, to);
		} catch (...) {
			closeSocket(h, current);
			throw;
		}
	}

	MAYBE(NetStream *) connect(Str *host, Nat port) {
		Array<Address *> *found = lookupAddress(host);

		for (Nat i = 0; i < found->count(); i++) {
			Address *addr = found->at(i);
			if (!addr->port())
				addr = addr->withPort(port);

			if (NetStream *s = connect(addr))
				return s;
		}

		return null;
	}


	/**
	 * IStream.
	 */

	NetIStream::NetIStream(NetStream *owner, os::Thread t) : HandleIStream(owner->handle, t), owner(owner) {}

	NetIStream::~NetIStream() {
		// The socket will close the handle.
		handle = os::Handle();
	}

	void NetIStream::close() {
		owner->closeEnd(NetStream::closeRead);
		handle = os::Handle();
	}


	/**
	 * OStream.
	 */

	NetOStream::NetOStream(NetStream *owner, os::Thread t) : HandleOStream(owner->handle, t), owner(owner) {}

	NetOStream::~NetOStream() {
		// The socket will close the handle.
		handle = os::Handle();
	}

	void NetOStream::close() {
		owner->closeEnd(NetStream::closeWrite);
		handle = os::Handle();
	}


}
