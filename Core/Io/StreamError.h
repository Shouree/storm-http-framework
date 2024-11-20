#pragma once
#include "Core/Exception.h"

namespace storm {
	STORM_PKG(core.io);

	/**
	 * Exception thrown for IO errors.
	 *
	 * Note that this is only used for major errors. End-of-stream and failure to write are not
	 * treated as major errors, and are handled through return codes instead. Exceptions indicate
	 * major problems with the IO system as a whole.
	 */
	class EXCEPTION_EXPORT IoError : public Exception {
		STORM_EXCEPTION;
	public:
		IoError(const wchar *msg) {
			w = new (this) Str(msg);
			saveTrace();
		}
		STORM_CTOR IoError(Str *msg) {
			w = msg;
			saveTrace();
		}

		virtual void STORM_FN message(StrBuf *to) const {
			*to << w;
		}
	private:
		Str *w;
	};


	namespace sys {
		/**
		 * System error codes for IO streams. Available from the `error` member of input- and output
		 * streams.
		 */
		enum ErrorCode {
			// No error. Use `any` or `empty` to check conveniently.
			none = 0,

			// Unknown error.
			unknown,

			// Low-level IO error.
			ioError,

			// The file is too big for the underlying file system to handle.
			tooLargeFile,

			// Out of space on the underlying device.
			outOfSpace,

			// The file (or a part of the file) was locked.
			fileLocked,

			// Remote end of pipe disconnected.
			disconnected,

			// Stream closed.
			closed,
		};

		// See if there was any error?
		inline Bool STORM_FN any(ErrorCode c) {
			return c != none;
		}
		inline Bool STORM_FN empty(ErrorCode c) {
			return c == none;
		}

		// Get a description of the error.
		Str *STORM_FN description(EnginePtr e, ErrorCode c);

		// Throw an error code as an exception, unless it is `none`.
		void STORM_FN throwError(EnginePtr e, ErrorCode c);
	}

	// Convert from system error code.
	sys::ErrorCode fromSystemError(int error);

}
