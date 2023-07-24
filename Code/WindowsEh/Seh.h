#pragma once

#ifdef WINDOWS

namespace code {
	namespace eh {

#ifdef X64

		// Callback function from the GC.
		RUNTIME_FUNCTION *exceptionCallback(void *pc, void *base);

#endif

	}
}

#endif
