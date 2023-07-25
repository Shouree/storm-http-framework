#pragma once
#include "Utils/Platform.h"

#ifdef WINDOWS

namespace code {
	namespace eh {

		/**
		 * Windows-specific exception handling.
		 */

		// Activate stack information. Needed for stack traces, etc.
		void activateWindowsInfo(Engine &e);

		// Exception handler function.
		EXCEPTION_DISPOSITION windowsHandler(_EXCEPTION_RECORD *er, void *frame, _CONTEXT *ctx, void *dispatch);
	}
}

#endif
