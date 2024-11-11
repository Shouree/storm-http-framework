#include "stdafx.h"
#include "Listener.h"

namespace storm {

	Listener::Listener(os::Handle socket, os::Thread attached) : Socket(socket, attached) {}

	NetStream *Listener::accept() {
		if (handle == os::Handle())
			return null;

		sockaddr_storage addr;
		os::Handle accepted = acceptSocket(handle, attachedTo, (sockaddr *)&addr, sizeof(addr));

		if (accepted) {
			if (Address *stormAddr = toStormUnsafe(engine(), (sockaddr *)&addr)) {
				attachedTo.attach(accepted);
				return new (this) NetStream(accepted, attachedTo, stormAddr);
			}
			closeSocket(accepted, attachedTo);
		}

		return null;
	}

	void Listener::toS(StrBuf *to) const {
		Socket::toS(to);
		*to << S(" (listening)");
	}

	static Listener *listen(EnginePtr e, sockaddr *addr, int addrSize, Bool reuseAddr) {
		initSockets();

		os::Handle socket = createTcpSocket(addr->sa_family);

		// TODO: Maybe not use SO_REUSEADDR on Windows?
		int reuse = reuseAddr ? 1 : 0;
		if (!setSocketOpt(socket, SOL_SOCKET, SO_REUSEADDR, &reuse, sizeof(int)))
			goto error;

		if (addr->sa_family == AF_INET6) {
			// Allow both ipv4 and ipv6.
			int disable = 0;
			if (!setSocketOpt(socket, IPPROTO_IPV6, IPV6_V6ONLY, &disable, sizeof(int)))
				goto error;
		}

		if (!bindSocket(socket, addr, addrSize))
			goto error;

		if (!listenSocket(socket, 20))
			goto error;

		{
			os::Thread thread = os::Thread::current();
			thread.attach(socket);
			return new (e.v) Listener(socket, thread);
		}

	error:
		closeSocket(socket, os::Thread::invalid);
		return null;
	}

	Listener *listen(EnginePtr e, Nat port) {
		return listen(e, port, true);
	}

	Listener *listen(EnginePtr e, Nat port, Bool reuseAddr) {
		sockaddr_in6 addr;
		memset(&addr, 0, sizeof(addr));
		addr.sin6_family = AF_INET6;
		addr.sin6_port = htons(short(port));
		addr.sin6_addr = in6addr_any;

		return listen(e, (sockaddr *)&addr, sizeof(addr), reuseAddr);
	}

	Listener *listen(Address *addr) {
		return listen(addr, true);
	}

	Listener *listen(Address *addr, Bool reuseAddr) {
		sockaddr_storage a;
		addr->fill((sockaddr *)&a);

		return listen(addr->engine(), (sockaddr *)&a, sizeof(a), reuseAddr);
	}

}
