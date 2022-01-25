#include "stdafx.h"
#include "IOCondition.h"
#include <limits>

#ifdef POSIX
// NOTE: This does not exist on all POSIX systems (eg. MacOS)
#include <sys/eventfd.h>
#endif

namespace os {

#ifdef WINDOWS

	IOCondition::IOCondition() : signaled(0) {
		sema = CreateSemaphore(NULL, 0, 1, NULL);
	}

	IOCondition::~IOCondition() {
		CloseHandle(sema);
	}

	void IOCondition::signal() {
		// If we're the first one to signal, alter the semaphore.
		if (atomicCAS(signaled, 0, 1) == 0)
			ReleaseSemaphore(sema, 1, NULL);
	}

	void IOCondition::wait() {
		// Wait for someone to signal, and then reset the signaled state for next time.
		WaitForSingleObject(sema, INFINITE);
		atomicCAS(signaled, 1, 0);
	}

	void IOCondition::wait(IOHandle &io) {
		HANDLE ioHandle = io.v();
		HANDLE handles[2] = { sema, ioHandle };
		DWORD r = WaitForMultipleObjects(ioHandle ? 2 : 1, handles, FALSE, INFINITE);
		atomicCAS(signaled, 1, 0);
	}

	bool IOCondition::wait(nat msTimeout) {
		DWORD result = WaitForSingleObject(sema, msTimeout);
		if (result == WAIT_OBJECT_0) {
			atomicCAS(signaled, 1, 0);
			return true;
		} else {
			return false;
		}
	}

	bool IOCondition::wait(IOHandle &io, nat msTimeout) {
		HANDLE ioHandle = io.v();
		HANDLE handles[2] = { sema, ioHandle };
		DWORD result = WaitForMultipleObjects(ioHandle ? 2 : 1, handles, FALSE, msTimeout);
		if (result == WAIT_OBJECT_0 || result == WAIT_OBJECT_0+1) {
			atomicCAS(signaled, 1, 0);
			return true;
		} else {
			return false;
		}
	}

#endif

#ifdef POSIX

	IOCondition::IOCondition() : signaled(0), fd(-1) {
		fd = eventfd(0, EFD_CLOEXEC | EFD_NONBLOCK);
	}

	IOCondition::~IOCondition() {
		close(fd);
	}

	void IOCondition::signal() {
		if (atomicCAS(signaled, 0, 1) == 0) {
			uint64_t val = 1;
			while (true) {
				ssize_t r = write(fd, &val, 8);
				if (r >= 0)
					break;
				if (errno == EAGAIN || errno == EINTR)
					continue;
				perror("Failed to signal eventfd");
			}
		}
	}

	bool IOCondition::doWait(struct pollfd *fds, size_t fdCount, int timeout) {
		fds[0].fd = fd;
		fds[0].events = POLLIN;
		fds[0].revents = 0;

		int result = -1;
		while (result < 0) {
			result = poll(fds, fdCount, timeout);

			if (result < 0) {
				if (errno == EINTR) {
					// TODO: We could make a better estimation.
					if (timeout > 0)
						timeout = 0;
					continue;
				}
				perror("poll");
				assert(false);
			}
		}

		if (result) {
			// Some entry is done. If it is entry #0, we want to read it so that it is not signaled anymore.
			if (fds[0].revents != 0) {
				uint64_t v = 0;
				ssize_t r = read(fd, &v, 8);
				if (r <= 0)
					perror("Failed to read from eventfd");
			}
		}

		// Now that we're done messing with the eventfd, we need to tell the world that they need to
		// signal the eventfd if they try to wake us again.
		atomicWrite(signaled, 0);

		// 'result == 0' => timeout, otherwise something interesting happened.
		return result != 0;
	}

	void IOCondition::wait() {
		struct pollfd wait;
		doWait(&wait, 1, -1);
	}

	void IOCondition::wait(IOHandle &io) {
		IOHandle::Desc desc = io.desc();
		doWait(desc.fds, desc.count, -1);
	}

	bool IOCondition::wait(nat msTimeout) {
		msTimeout = min(msTimeout, nat(std::numeric_limits<int>::max()));
		struct pollfd wait;
		return doWait(&wait, 1, msTimeout);
	}

	bool IOCondition::wait(IOHandle &io, nat msTimeout) {
		IOHandle::Desc desc = io.desc();
		return doWait(desc.fds, desc.count, msTimeout);
	}

#endif

}
