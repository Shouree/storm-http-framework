#pragma once

#include "Utils/Platform.h"

// Does this platform provide callbacks for exception handling from the GC? If so, then
// STORM_GC_EH_CALLBACK is defined to a function pointer type to be used.


#if defined(WINDOWS) && defined(X64)

#include "Utils/Win32.h"

namespace storm {

	typedef RUNTIME_FUNCTION *(*WinEhCallback)(void *pc, void *base);
}

#define STORM_GC_EH_CALLBACK WinEhCallback
#endif
