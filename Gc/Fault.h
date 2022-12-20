#pragma once

#ifdef POSIX
#include <signal.h>

namespace storm {


	/**
	 * Handle segmentation faults on Posix.
	 */

	void setSegvHandler(void (*)(int, siginfo_t *, void *));

	// Called by GC:s when they don't handle SIGSEGV.
	extern "C" void gcHandleSegv(int signal, siginfo_t *info, void *context);

}

#endif
