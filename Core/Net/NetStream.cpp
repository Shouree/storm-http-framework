#include "stdafx.h"
#include "NetStream.h"

#if defined(WINDOWS)
#include <mstcpip.h> // for tcp_keepalive
#endif

namespace storm {

	void Keepalive::toS(StrBuf *to) const {
		if (!enabled) {
			*to << S("<keepalive off>");
		} else if (idle.v <= 0 || interval.v <= 0) {
			*to << S("<keepalive on, default times>");
		} else {
			*to << S("<keepalive on, idle: ") << idle << S(", interval: ") << interval << S(">");
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

	Keepalive NetStream::keepalive() const {
		return alive;
	}

#if defined(WINDOWS)

	void NetStream::keepalive(Keepalive d) {
		alive = d;

		Bool ok = false;

		if (!alive.enabled) {
			// Just turn it off with a setsockopt.
			int enabled = 1;
			ok = setSocketOpt(handle, SOL_SOCKET, SO_KEEPALIVE, &enabled, sizeof(enabled));
		} else if (alive.enabled && !alive.defaultTimes()) {
			// Just turn it on with a setsockopt.
			int enabled = 0;
			ok = setSocketOpt(handle, SOL_SOCKET, SO_KEEPALIVE, &enabled, sizeof(enabled));
		} else {
			// Set times also:
			tcp_keepalive info = {
				1,
				ULONG(alive.idle.inMs()),
				ULONG(alive.interval.inMs()),
			};
			DWORD out = 0;
			int res = WSAIoctl((SOCKET)handle.v(), SIO_KEEPALIVE_VALS,
							&info, sizeof(info), // input
							NULL, NULL, &out, // no output
							NULL, NULL); // not OVERLAPPED
			ok = res == 0;
		}

		if (!ok)
			throw new (this) NetError(S("Failed to set keepalive information."));
	}

#elif defined(LINUX)

	void NetStream::keepalive(Keepalive d) {
		alive = d;

		int enabled = alive.enabled ? 1 : 0;
		if (!setSocketOpt(handle, SOL_SOCKET, SO_KEEPALIVE, &enabled, sizeof(enabled)))
			throw new (this) NetError(S("Failed to set SO_KEEPALIVE."));

		if (alive.enabled && !alive.defaultTimes()) {
			int idletime = alive.idle.inS();
			int idleinterval = alive.interval.inS();

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

	NetIStream::NetIStream(NetStream *owner, os::Thread t) : HandleTimeoutIStream(owner->handle, t), owner(owner) {}

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
