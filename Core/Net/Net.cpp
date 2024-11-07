#include "stdafx.h"
#include "Net.h"
#include "OS/IORequest.h"

namespace storm {

#if defined(WINDOWS)

	class WS {
	public:
		WS() {
			WSADATA data;
			if (WSAStartup(MAKEWORD(2, 2), &data)) {
				assert(false, L"Unable to initialize sockets.");
			}
		}

		~WS() {
			WSACleanup();
		}
	};

	void initSockets() {
		static WS data;
	}

#else

	// No other system currently requires initialization to use sockets.
	void initSockets() {}

#endif


#if defined(WINDOWS)

	static os::Handle createSocket(int af, int mode, int proto) {
		SOCKET s = WSASocket(af, mode, proto, NULL, 0, WSA_FLAG_OVERLAPPED);

		// Additional setup?

		return os::Handle((HANDLE)s);
	}

	bool bindSocket(os::Handle socket, const sockaddr *name, socklen_t nameSize) {
		return bind((SOCKET)socket.v(), name, nameSize) == 0;
	}

	bool listenSocket(os::Handle socket, int backlog) {
		return listen((SOCKET)socket.v(), backlog) == 0;
	}

	static BOOL AcceptEx(os::Handle listen, os::Handle accept,
						void *output, DWORD receiveLen,
						DWORD localLen, DWORD remoteLen,
						DWORD *received, OVERLAPPED *overlapped) {

		SOCKET s = (SOCKET)listen.v();
		LPFN_ACCEPTEX ptr = null;
		GUID guid = WSAID_ACCEPTEX;
		DWORD bytes = 0;
		int ok = WSAIoctl(s, SIO_GET_EXTENSION_FUNCTION_POINTER, &guid, sizeof(guid), &ptr, sizeof(ptr), &bytes, NULL, NULL);

		assert(ok == 0, L"Unable to acquire AcceptEx.");

		return (*ptr)(s, (SOCKET)accept.v(), output, receiveLen, localLen, remoteLen, received, overlapped);
	}

	static void GetAcceptExSockaddrs(os::Handle socket, void *buffer, DWORD receiveLen,
									DWORD localLen, DWORD remoteLen,
									LPSOCKADDR *localAddr, int *localSize,
									LPSOCKADDR *remoteAddr, int *remoteSize) {

		SOCKET s = (SOCKET)socket.v();
		LPFN_GETACCEPTEXSOCKADDRS ptr = null;
		GUID guid = WSAID_GETACCEPTEXSOCKADDRS;
		DWORD bytes = 0;
		int ok = WSAIoctl(s, SIO_GET_EXTENSION_FUNCTION_POINTER, &guid, sizeof(guid), &ptr, sizeof(ptr), &bytes, NULL, NULL);

		assert(ok == 0, L"Unable to acquire GetAcceptExSockaddrs.");

		return (*ptr)(buffer, receiveLen, localLen, remoteLen, localAddr, localSize, remoteAddr, remoteSize);
	}

	// Additional 16 bytes as required by AcceptEx (see the documentation for details).
	struct sockaddr_large {
		sockaddr_storage addr;
		byte extra[16];
	};

	os::Handle acceptSocket(os::Handle socket, const os::Thread &thread, sockaddr *addr, socklen_t addrSize) {
		os::Handle out = createSocket(AF_INET6, SOCK_STREAM, IPPROTO_TCP);

		// Output buffer with some extra data as specified in the documentation.
		sockaddr_large buffer[2];
		os::IORequest request(socket, thread);
		DWORD bytes = 0;

		int error = 0;
		if (!AcceptEx(socket, out, &buffer, 0, sizeof(sockaddr_large), sizeof(sockaddr_large), &bytes, &request))
			error = WSAGetLastError();

		if (error == ERROR_IO_PENDING || error == 0) {
			// Wait for the result.
			request.wake.down();

			if (request.error == 0) {
				sockaddr *local, *remote;
				int localSize, remoteSize;

				GetAcceptExSockaddrs(socket, buffer, 0, sizeof(sockaddr_large), sizeof(sockaddr_large),
									&local, &localSize, &remote, &remoteSize);

				memcpy(addr, remote, min(addrSize, remoteSize));

				return out;
			}
		}

		// Some other error.
		closeSocket(out, thread);
		return os::Handle();
	}

	static BOOL ConnectEx(os::Handle socket, const sockaddr *name, int namelen, OVERLAPPED *overlapped) {
		SOCKET s = (SOCKET)socket.v();
		LPFN_CONNECTEX ptr = null;
		GUID guid = WSAID_CONNECTEX;
		DWORD bytes = 0;
		int ok = WSAIoctl(s, SIO_GET_EXTENSION_FUNCTION_POINTER, &guid, sizeof(guid), &ptr, sizeof(ptr), &bytes, NULL, NULL);

		assert(ok == 0, L"Unable to acquire ConnectEx.");

		return (*ptr)(s, name, namelen, NULL, 0, NULL, overlapped);
	}

	bool connectSocket(os::Handle socket, const os::Thread &attached, sockaddr *addr, socklen_t addrSize) {
		// We need to bind the socket to use 'ConnectEx'...
		sockaddr_storage any;
		memset(&any, 0, sizeof(any));
		any.ss_family = addr->sa_family;
		if (bind((SOCKET)socket.v(), (sockaddr *)&any, sizeof(any))) {
			return false;
		}

		os::IORequest request(socket, attached);

		int error = 0;
		if (!ConnectEx(socket, addr, addrSize, &request))
			error = WSAGetLastError();


		if (error == ERROR_IO_PENDING || error == 0) {
			// Wait for the result.
			request.wake.down();

			return request.error == 0;
		} else {
			// Failed in some other way.
			return false;
		}

		return false;
	}

	void closeSocket(os::Handle handle, const os::Thread &attached) {
		closesocket((SOCKET)handle.v());
	}

	bool getSocketName(os::Handle handle, sockaddr *out, socklen_t len) {
		return getsockname((SOCKET)handle.v(), out, &len) == 0;
	}

	bool getSocketOpt(os::Handle handle, int level, int name, void *value, socklen_t size) {
		int v = size;
		return getsockopt((SOCKET)handle.v(), level, name, (char *)value, &v) == 0;
	}

	bool setSocketOpt(os::Handle handle, int level, int name, void *value, socklen_t size) {
		return setsockopt((SOCKET)handle.v(), level, name, (char *)value, size) == 0;
	}

	Duration getSocketTime(os::Handle handle, int level, int name) {
		DWORD out = 0;
		getSocketOpt(handle, level, name, &out, sizeof(out));
		return time::ms(out);
	}

	bool setSocketTime(os::Handle handle, int level, int name, const Duration &d) {
		DWORD out = (DWORD)d.inMs();
		return setSocketOpt(handle, level, name, &out, sizeof(out));
	}


#elif defined(POSIX)

	static os::Handle createSocket(int af, int mode, int proto) {
		return socket(af, mode | SOCK_NONBLOCK | SOCK_CLOEXEC, proto);
	}

	bool bindSocket(os::Handle socket, const sockaddr *addr, socklen_t addrSize) {
		return bind(socket.v(), addr, addrSize) == 0;
	}

	bool listenSocket(os::Handle socket, int backlog) {
		return listen(socket.v(), backlog) == 0;
	}

	// Returns 'false' if the file descriptor was closed.
	static bool doWait(os::Handle h, const os::Thread &attached, os::IORequest::Type type) {
		os::IORequest request(h, type, attached);
		request.wake.wait();
		return !request.closed;
	}

	static bool setNonblocking(int fd) {
		int flags = fcntl(fd, F_GETFL, 0);

		if (flags == -1)
			return false;

		if (fcntl(fd, F_SETFL, flags | O_NONBLOCK | O_CLOEXEC) == -1)
			return false;

		return true;
	}

	os::Handle acceptSocket(os::Handle socket, const os::Thread &attached, sockaddr *addr, socklen_t addrSize) {
		while (true) {
			int fd = accept(socket.v(), addr, &addrSize);
			if (fd >= 0) {
				if (!setNonblocking(fd)) {
					perror("fcntl");
					close(fd);
					return os::Handle();
				}

				// Ready to go!
				return os::Handle(fd);
			}

			if (errno == EINTR) {
				// Aborted by a signal. Retry.
				continue;
			} else if (errno == EAGAIN) {
				// Wait for more data.
				if (!doWait(socket, attached, os::IORequest::read))
					break;
			} else {
				// Unknown error.
				break;
			}
		}

		return os::Handle();
	}

	bool connectSocket(os::Handle socket, const os::Thread &attached, sockaddr *addr, socklen_t addrSize) {
		int ok = 0;

		// Try to connect. Repeat until we're not interrupted.
		while ((ok = connect(socket.v(), addr, addrSize)) != 0) {
			if (errno == EINTR) {
				// Aborted by signal. Retry.
				continue;
			} else if (errno == EINPROGRESS || errno == EALREADY) {
				// Connection in progress. Wait for that to complete!
				break;
			} else {
				// Unknown error. Abort.
				return false;
			}
		}

		// If 'ok' indicates an error, we should wait.
		if (ok != 0) {
			if (!doWait(socket, attached, os::IORequest::write)) {
				// Closed by us before completion (should not happen).
				return false;
			}

			// Acquire the result of the connect operation.
			int error = 0;
			socklen_t len = sizeof(int);
			if (getsockopt(socket.v(), SOL_SOCKET, SO_ERROR, &error, &len)) {
				perror("getsockopt");
				return false;
			}

			if (error != 0) {
				// PVAR(error);
				return false;
			}
		}

		// We're good to go!
		return true;
	}

	void closeSocket(os::Handle handle, const os::Thread &attached) {
		if (attached != os::Thread::invalid)
			attached.detach(handle);
		close(handle.v());
	}

	bool getSocketName(os::Handle socket, sockaddr *out, socklen_t size) {
		return getsockname(socket.v(), out, &size) == 0;
	}

	bool getSocketOpt(os::Handle socket, int level, int name, void *value, socklen_t size) {
		return getsockopt(socket.v(), level, name, value, &size) == 0;
	}

	bool setSocketOpt(os::Handle socket, int level, int name, void *value, socklen_t size) {
		return setsockopt(socket.v(), level, name, value, size) == 0;
	}

	Duration getSocketTime(os::Handle socket, int level, int name) {
		struct timeval tv;
		memset(&tv, 0, sizeof(tv));
		if (!getSocketOpt(socket, level, name, &tv, sizeof(tv)))
			return Duration();

		return time::us(tv.tv_usec) + time::s(tv.tv_sec);
	}

	bool setSocketTime(os::Handle socket, int level, int name, const Duration &d) {
		struct timeval tv;
		tv.tv_usec = d.inUs() % 1000000;
		tv.tv_sec = d.inS();

		return setSocketOpt(socket, level, name, &tv, sizeof(tv));
	}

#else
#error "Please implement sockets for your platform!"
#endif

	os::Handle createTcpSocket(int family) {
		return createSocket(family, SOCK_STREAM, IPPROTO_TCP);
	}

	os::Handle createUdpSocket(int family) {
		return createSocket(family, SOCK_DGRAM, IPPROTO_UDP);
	}


}
