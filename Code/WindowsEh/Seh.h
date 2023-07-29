#pragma once
#include "Utils/Platform.h"
#include "Code/Binary.h"

#ifdef WINDOWS

namespace code {
	namespace eh {

		/**
		 * Windows-specific exception handling.
		 */

		// Activate stack information. Needed for stack traces, etc.
		void activateWindowsInfo(Engine &e);

		// Exception handler function. Common to both 32- and 64-bit systems.
		extern "C"
		EXCEPTION_DISPOSITION windowsHandler(_EXCEPTION_RECORD *er, void *frame, _CONTEXT *ctx, void *dispatch);


		// Interface used by 'windowsHandler'. Implemented by either Seh32.cpp or Seh64.cpp.
		struct SehFrame {
			// Pointer to the stack:
			void *framePtr;

			// Pointer to the Binary containing metadata:
			Binary *binary;

			// Current active block and activation.
			Nat part;
			Nat activation;
		};

		// Get a SEH frame from unwind state.
		SehFrame extractFrame(_EXCEPTION_RECORD *er, void *frame, _CONTEXT *ctx, void *dispatch);

		// Modify state to resume from an exception.
		void resumeFrame(SehFrame &frame, Binary::Resume &resume, storm::RootObject *object,
						_CONTEXT *ctx, _EXCEPTION_RECORD *er);

	}
}

/**
 * Things missing from the Win32 headers:
 */

#ifndef EXCEPTION_UNWINDING
#define EXCEPTION_UNWINDING 0x2
#endif
#ifndef EXCEPTION_NONCONTINUABLE
#define EXCEPTION_NONCONTINUABLE 0x1
#endif
#ifndef EXCEPTION_TARGET_UNWIND
#define EXCEPTION_TARGET_UNWIND 0x20
#endif

#endif
