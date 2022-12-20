#include "stdafx.h"
#include "Fault.h"

#ifdef POSIX

namespace storm {

	static void (*segvHandler)(int, siginfo_t *, void *);

	void setSegvHandler(void (*handler)(int, siginfo_t *, void *)) {
		segvHandler = handler;
	}

	void gcHandleSegv(int signal, siginfo_t *info, void *context) {
		if (segvHandler)
			(*segvHandler)(signal, info, context);
		else
			raise(SIGINT);
	}

}

#endif
