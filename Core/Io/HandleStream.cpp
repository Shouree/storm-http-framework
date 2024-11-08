#include "stdafx.h"
#include "HandleStream.h"
#include "Core/Exception.h"
#include "OS/IORequest.h"
#include <limits>

namespace storm {

	// Helper to make a Duration into a ms interval, clamping as necessary.
	static nat toMs(Duration duration) {
		Long ms = duration.inMs();
		if (ms < 0)
			return 0;
		if (ms > std::numeric_limits<nat>::max())
			return std::numeric_limits<nat>::max();
		return nat(ms);
	}

	/**
	 * System-specific helpers. These all behave as if the handle was blocking.
	 */

#if defined(WINDOWS)

	static inline void close(os::Handle &h, os::Thread &attached) {
		CloseHandle(h.v());
		attached = os::Thread::invalid;
		h = os::Handle();
	}

	static Nat read(os::Handle h, os::Thread &attached, void *dest, Nat limit, Duration timeout = Duration()) {
		if (attached == os::Thread::invalid) {
			attached = os::Thread::current();
			attached.attach(h);
		}

		os::IORequest request(h, attached, toMs(timeout));

		LARGE_INTEGER pos;
		pos.QuadPart = 0;
		if (SetFilePointerEx(h.v(), pos, &pos, FILE_CURRENT)) {
			// There seems to be a poblem when reading from the end of a file asynchronously.
			LARGE_INTEGER len;
			GetFileSizeEx(h.v(), &len);
			if (pos.QuadPart >= len.QuadPart)
				return 0;

			request.Offset = pos.LowPart;
			request.OffsetHigh = pos.HighPart;
		} else {
			// If we can not seek, it means that the offset should be zero.
			request.Offset = 0;
			request.OffsetHigh = 0;
		}

		int error = 0;
		// Note: When using ReadFile with sockets, it will sometimes indicate synchronous
		// completion *and* notify the IO completion port. Therefore, we will not trust the return
		// value, and assume asynchronous completion anyway.
		if (!ReadFile(h.v(), dest, DWORD(limit), NULL, &request))
			error = GetLastError();

		if (error == ERROR_IO_PENDING || error == 0) {
			// Completing async...
			request.wake.down();

			// Advance the file pointer.
			pos.QuadPart = request.bytes;
			SetFilePointerEx(h.v(), pos, NULL, FILE_CURRENT);
		} else {
			// Failed.
			request.bytes = 0;
		}

		return request.bytes;
	}

	static Nat write(os::Handle h, os::Thread &attached, const void *src, Nat limit) {
		if (attached == os::Thread::invalid) {
			attached = os::Thread::current();
			attached.attach(h);
		}

		os::IORequest request(h, attached, 0);

		LARGE_INTEGER pos;
		pos.QuadPart = 0;
		if (SetFilePointerEx(h.v(), pos, &pos, FILE_CURRENT)) {
			// All is well.
			request.Offset = pos.LowPart;
			request.OffsetHigh = pos.HighPart;
		} else {
			// If we can not seek, it means that the offset should be zero.
			request.Offset = 0;
			request.OffsetHigh = 0;
		}

		int error = 0;
		// Note: When using WriteFile with sockets, it will sometimes indicate synchronous
		// completion *and* notify the IO completion port. Therefore, we will not trust the return
		// value, and assume asynchronous completion anyway.
		if (!WriteFile(h.v(), src, DWORD(limit), NULL, &request))
			error = GetLastError();

		if (error == ERROR_IO_PENDING || error == 0) {
			// Completing async...
			request.wake.down();

			// Advance the file pointer.
			pos.QuadPart = request.bytes;
			SetFilePointerEx(h.v(), pos, NULL, FILE_CURRENT);
		} else {
			// Failed.
			request.bytes = 0;
		}

		return request.bytes;
	}

	static void seek(os::Handle h, Word to) {
		LARGE_INTEGER pos;
		pos.QuadPart = to;
		SetFilePointerEx(h.v(), pos, NULL, FILE_BEGIN);
	}

	static Word tell(os::Handle h) {
		LARGE_INTEGER pos;
		pos.QuadPart = 0;
		SetFilePointerEx(h.v(), pos, &pos, FILE_CURRENT);
		return pos.QuadPart;
	}

	static Word length(os::Handle h) {
		LARGE_INTEGER len;
		GetFileSizeEx(h.v(), &len);
		return len.QuadPart;
	}

	static os::Handle dupHandle(os::Handle src) {
		if (!src)
			return os::Handle();

		HANDLE dest = INVALID_HANDLE_VALUE;
		HANDLE proc = GetCurrentProcess();
		if (!DuplicateHandle(proc, src.v(), proc, &dest, DUPLICATE_SAME_ACCESS, FALSE, 0)) {
			PLN(L"Failed to duplicate handle: " << GetLastError());
			return os::Handle();
		}

		return dest;
	}

	static os::Handle openStd(DWORD id, bool input) {
		return dupHandle(GetStdHandle(id));
	}

#elif defined(POSIX)

	static inline void close(os::Handle &h, os::Thread &attached) {
		if (attached != os::Thread::invalid)
			attached.detach(h);

		::close(h.v());
		attached = os::Thread::invalid;
		h = os::Handle();
	}

	// Returns 'false' if the handle was closed (by us) during the operation.
	static bool doWait(os::Handle h, os::Thread &attached, os::IORequest::Type type, Duration timeout) {
		if (attached == os::Thread::invalid) {
			attached = os::Thread::current();
			attached.attach(h);
		}

		os::IORequest request(h, type, attached, toMs(timeout));
		request.wake.wait();
		return !request.closed;
	}

	static Nat read(os::Handle h, os::Thread &attached, void *dest, Nat limit, Duration timeout = Duration()) {
		while (true) {
			ssize_t r = ::read(h.v(), dest, size_t(limit));
			if (r >= 0)
				return Nat(r);

			if (errno == EINTR) {
				// Aborted by a signal. Retry.
				continue;
			} else if (errno == EAGAIN) {
				// Wait for more data.
				if (!doWait(h, attached, os::IORequest::read, timeout))
					break;
			} else {
				// Unknown error.
				break;
			}
		}

		return 0;
	}

	static Nat write(os::Handle h, os::Thread &attached, const void *src, Nat limit) {
		while (true) {
			ssize_t r = ::write(h.v(), src, size_t(limit));
			if (r >= 0)
				return Nat(r);

			if (errno == EINTR) {
				// Aborted by a signal. Retry.
				continue;
			} else if (errno == EAGAIN) {
				// Wait for more data.
				if (!doWait(h, attached, os::IORequest::write, Duration()))
					break;
			} else {
				// Unknown error.
				break;
			}
		}

		return 0;
	}

	static void seek(os::Handle h, Word to) {
		lseek64(h.v(), to, SEEK_SET);
	}

	static Word tell(os::Handle h) {
		off64_t r = lseek64(h.v(), 0, SEEK_CUR);
		if (r < 0)
			return 0;
		return Word(r);
	}

	static Word length(os::Handle h) {
		off64_t old = lseek64(h.v(), 0, SEEK_CUR);
		if (old < 0)
			return 0;

		off64_t size = lseek64(h.v(), 0, SEEK_END);
		lseek64(h.v(), old, SEEK_SET);
		return Word(size);
	}

	static os::Handle dupHandle(os::Handle src) {
		return os::Handle(dup(src.v()));
	}

#else
#error "Please implement file IO for your OS."
#endif

	/**
	 * Regular input stream.
	 */

	HandleIStream::HandleIStream(os::Handle h)
		: handle(h),
		  attachedTo(os::Thread::invalid) {}

	HandleIStream::HandleIStream(os::Handle h, os::Thread t)
		: handle(h),
		  attachedTo(t) {}

	// HandleIStream::HandleIStream(const HandleIStream &o)
	// 	: PeekIStream(o),
	// 	  handle(dupHandle(o.handle)),
	// 	  attachedTo(os::Thread::invalid) {}

	HandleIStream::HandleIStream(const HandleIStream &o) : PeekIStream(o), attachedTo(os::Thread::invalid) {
		throw new (this) NotSupported(S("Copying HandleIStream"));
	}

	HandleIStream::~HandleIStream() {
		if (handle)
			storm::close(handle, attachedTo);
	}

	Bool HandleIStream::more() {
		if (!handle)
			return false;

		return PeekIStream::more();
	}

	void HandleIStream::close() {
		if (handle)
			storm::close(handle, attachedTo);

		PeekIStream::close();
	}

	Nat HandleIStream::doRead(byte *to, Nat count) {
		if (handle)
			return storm::read(handle, attachedTo, to, count);
		else
			return 0;
	}

	/**
	 * Random access stream.
	 */

	HandleRIStream::HandleRIStream(os::Handle h)
		: handle(h),
		  attachedTo(os::Thread::invalid) {}

	HandleRIStream::HandleRIStream(os::Handle h, os::Thread t)
		: handle(h),
		  attachedTo(t) {}

	// HandleRIStream::HandleRIStream(const HandleRIStream &o)
	// 	: handle(dupHandle(o.handle)),
	// 	  attachedTo(os::Thread::invalid) {}

	HandleRIStream::HandleRIStream(const HandleRIStream &o) : attachedTo(os::Thread::invalid) {
		throw new (this) NotSupported(S("Copying HandleIStream"));
	}

	HandleRIStream::~HandleRIStream() {
		if (handle)
			storm::close(handle, attachedTo);
	}

	void HandleRIStream::deepCopy(CloneEnv *e) {
		// Nothing needs to be done.
	}

	Bool HandleRIStream::more() {
		if (!handle)
			return false;

		return tell() < length();
	}

	Buffer HandleRIStream::read(Buffer b) {
		Nat start = b.filled();

		if (!handle)
			return b;

		if (start >= b.count())
			return b;

		Nat r = storm::read(handle, attachedTo, b.dataPtr() + start, b.count() - start);
		b.filled(r + start);
		return b;
	}

	Buffer HandleRIStream::peek(Buffer b) {
		if (!handle)
			return b;

		Word pos = tell();
		b = read(b);
		seek(pos);

		return b;
	}

	void HandleRIStream::close() {
		if (handle)
			storm::close(handle, attachedTo);
	}

	RIStream *HandleRIStream::randomAccess() {
		return this;
	}

	void HandleRIStream::seek(Word to) {
		storm::seek(handle, to);
	}

	Word HandleRIStream::tell() {
		if (!handle)
			return 0;

		return storm::tell(handle);
	}

	Word HandleRIStream::length() {
		if (!handle)
			return 0;

		return storm::length(handle);
	}

	/**
	 * With timeout.
	 */

	HandleTimeoutIStream::HandleTimeoutIStream(os::Handle handle) : HandleIStream(handle), timeout() {}

	HandleTimeoutIStream::HandleTimeoutIStream(os::Handle handle, os::Thread attachedTo) : HandleIStream(handle), timeout() {}

	Nat HandleTimeoutIStream::doRead(byte *to, Nat count) {
		if (handle)
			return storm::read(handle, attachedTo, to, count, timeout);
		else
			return 0;
	}

	/**
	 * Output stream.
	 */

	HandleOStream::HandleOStream(os::Handle h)
		: handle(h),
		  attachedTo(os::Thread::invalid) {}

	HandleOStream::HandleOStream(os::Handle h, os::Thread t)
		: handle(h),
		  attachedTo(t) {}

	// HandleOStream::HandleOStream(const HandleOStream &o)
	// 	: handle(dupHandle(o.handle)),
	// 	  attachedTo(os::Thread::invalid) {}

	HandleOStream::HandleOStream(const HandleOStream &o) : attachedTo(os::Thread::invalid) {
		throw new (this) NotSupported(S("Copying HandleIStream"));
	}

	HandleOStream::~HandleOStream() {
		if (handle)
			storm::close(handle, attachedTo);
	}

	void HandleOStream::write(Buffer to, Nat start) {
		start = min(start, to.filled());
		if (handle) {
			while (start < to.filled()) {
				Nat r = storm::write(handle, attachedTo, to.dataPtr() + start, to.filled() - start);
				if (r == 0)
					break;
				start += r;
			}
		}
	}

	void HandleOStream::close() {
		if (handle)
			storm::close(handle, attachedTo);
	}

}
