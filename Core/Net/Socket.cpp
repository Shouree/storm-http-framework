#include "stdafx.h"
#include "Socket.h"
#include "Core/Exception.h"

namespace storm {


	Socket::Socket(os::Handle handle, os::Thread attachedTo) : handle(handle), attachedTo(attachedTo) {
		assert(attachedTo != os::Thread::invalid, L"We need a thread!");
	}

	Socket::Socket(const Socket &o) : Object(o), attachedTo(os::Thread::invalid) {
		throw new (this) NotSupported(S("Copying a socket"));
	}

	Socket::~Socket() {
		if (handle)
			closeSocket(handle, attachedTo);
	}

	void Socket::close() {
		if (handle) {
			closeSocket(handle, attachedTo);
			handle = os::Handle();
		}
	}

	Nat Socket::inputBufferSize() const {
		Nat out = 0;
		getSocketOpt(handle, SOL_SOCKET, SO_RCVBUF, &out, sizeof(out));
		return out;
	}

	void Socket::inputBufferSize(Nat size) {
		setSocketOpt(handle, SOL_SOCKET, SO_RCVBUF, &size, sizeof(size));
	}

	Nat Socket::outputBufferSize() const {
		Nat out = 0;
		getSocketOpt(handle, SOL_SOCKET, SO_SNDBUF, &out, sizeof(out));
		return out;
	}

	void Socket::outputBufferSize(Nat size) {
		setSocketOpt(handle, SOL_SOCKET, SO_SNDBUF, &size, sizeof(size));
	}

	void Socket::toS(StrBuf *to) const {
		*to << S("Socket: ");
		if (handle) {
			sockaddr_storage data;
			memset(&data, 0, sizeof(data));
			if (getSocketName(handle, (sockaddr *)&data, sizeof(data))) {
				*to << toStorm(engine(), (sockaddr *)&data);
			} else {
				*to << S("<unknown>");
			}
		} else {
			*to << S("<closed>");
		}
	}

}
