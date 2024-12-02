#include "stdafx.h"
#include "StreamError.h"
#include "Core/Str.h"

namespace storm {

#define ERROR_DESCRIPTION(code, msg)			\
	if (c == sys::code)							\
		return new (e.v) Str(S(msg))

	Str *sys::description(EnginePtr e, sys::ErrorCode c) {
		ERROR_DESCRIPTION(none, "No error");
		ERROR_DESCRIPTION(unknown, "Unknown error");
		ERROR_DESCRIPTION(ioError, "Low-level IO error");
		ERROR_DESCRIPTION(tooLargeFile, "The file is too large for the device to handle");
		ERROR_DESCRIPTION(outOfSpace, "Out of space on the physical device");
		ERROR_DESCRIPTION(disconnected, "The remote end of the pipe or socket was disconnected");
		ERROR_DESCRIPTION(fileLocked, "This part of the file is locked by another process");
		ERROR_DESCRIPTION(closed, "The stream was closed");

		return new (e.v) Str(S("Unknown error code"));
	}

	void sys::throwError(EnginePtr e, sys::ErrorCode c) {
		if (c != sys::none)
			throw new (e.v) IoError(description(e, c));
	}

#if defined(WINDOWS)

	sys::ErrorCode fromSystemError(int error) {
		switch (error) {
		case ERROR_BROKEN_PIPE: return sys::disconnected;
		case ERROR_NETNAME_DELETED: return sys::disconnected;
		case ERROR_WRITE_FAULT: return sys::ioError;
		case ERROR_LOCK_VIOLATION: return sys::fileLocked;
		case ERROR_SHARING_VIOLATION: return sys::fileLocked;
		case ERROR_HANDLE_DISK_FULL: return sys::outOfSpace;
		case ERROR_DISK_FULL: return sys::outOfSpace;
		case ERROR_DISK_QUOTA_EXCEEDED: return sys::outOfSpace;
		case ERROR_FILE_TOO_LARGE: return sys::tooLargeFile;
		case ERROR_INVALID_HANDLE: return sys::closed;
		default:
			return sys::unknown;
		}
	}

#elif defined(POSIX)

	sys::ErrorCode fromSystemError(int error) {
		switch (error) {
		case EDQUOT: return sys::tooLargeFile;
		case EFBIG: return sys::tooLargeFile;
		case EIO: return sys::ioError;
		case ENOSPC: return sys::outOfSpace;
		case EPIPE: return sys::disconnected;
		case EPERM: return sys::fileLocked;
		case EBADF: return sys::closed;
		default:
			return sys::unknown;
		}
	}

#endif

}
