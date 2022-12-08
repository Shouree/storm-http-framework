#include "stdafx.h"
#include "SystemException.h"
#include "Utils/Lock.h"
#include "Gc/DwarfTable.h"
#include "Core/Exception.h"

#ifdef POSIX
#define XOPEN
#include <signal.h>
#endif

namespace storm {

#ifdef X64
	// TODO: Call code inside Code/ for this...
	RootObject *functionContext(const void *fnStart) {
		size_t size = runtime::codeSize(fnStart);
		if (s <= sizeof(void *) * 2)
			return null;

		const byte *end = (const byte *)fnStart + size;
		RootObject **data = (RootObject **)end;
		return data[-2];
	}
#endif

#ifdef X86
	RootObject *functionContext(const void *fnStart) {
		size_t s = runtime::codeSize(fnStart);
		if (s <= sizeof(void *))
			return null;

		const void *pos = (const char *)fnStart + s - sizeof(void *);
		return *(RootObject **)pos;
	}
#endif

	static Engine *findEngine(const void *pc) {
		FDE *fde = dwarfTable().find(pc);
		if (!fde)
			return null;

		RootObject *o = functionContext(fde->codeStart());
		if (!o)
			return null;

		return &o->engine();
	}

#ifdef POSIX

	static void chainSigHandler(struct sigaction &sig, int signal, siginfo_t *info, void *context) {
		if (sig.sa_flags & SA_SIGINFO) {
			(*sig.sa_sigaction)(signal, info, context);
		} else if (sig.sa_handler == SIG_DFL || sig.sa_handler == SIG_IGN) {
			// Dispatch through system mechanisms.
			sigset_t sigmask, oldsigmask;
			sigemptyset(&sigmask);
			sigaddset(&sigmask, signal);
			struct sigaction tmp;
			sigaction(signal, &sig, &tmp);
			sigprocmask(SIG_UNBLOCK, &sigmask, &oldsigmask);
			raise(signal);
			sigprocmask(SIG_SETMASK, &oldsigmask, NULL);
			sigaction(signal, &tmp, NULL);
		} else {
			(*sig.sa_handler)(signal);
		}
	}

	static struct sigaction oldFpeAction;

	static void handleFpe(int signal, siginfo_t *info, void *context) {
		// Try to find an engine based on the code that called the instruction, so that we can
		// extract an Engine object to instantiate the exception.
		Engine *e = findEngine(info->si_addr);
		if (!e)
			e = runtime::someEngineUnsafe();

		if (e) {
			switch (info->si_code) {
			case FPE_INTDIV:
				throw new (*e) DivisionByZero();
			default:
				// We could add more...
				break;
			}
		}

		// If we get here, dispatch to other signal handler. This might crash the process.
		chainSigHandler(oldFpeAction, signal, info, context);
	}

	static struct sigaction oldSegvAction;

	static void handleSegv(int signal, siginfo_t *info, void *context) {
		// Try to find an engine based on the code that called the instruction, so that we can
		// extract an Engine object to instantiate the exception.
		Engine *e = findEngine(info->si_addr);
		if (!e)
			e = runtime::someEngineUnsafe();

		if (e) {
			switch (info->si_code) {
			case SEGV_MAPERR:
			case SEGV_ACCERR:
				throw new (*e) MemoryAccessError(Word(info->si_addr));
			default:
				// We could add more...
				break;
			}
		}

		// If we get here, dispatch to other signal handler. This might crash the process.
		chainSigHandler(oldSegvAction, signal, info, context);
	}

	static void initialize() {
		// Add handler for SIGFPE
		struct sigaction sa;
		sa.sa_sigaction = &handleFpe;
		sigemptyset(&sa.sa_mask);
		sa.sa_flags = SA_RESTART | SA_SIGINFO;
		sigaction(SIGFPE, &sa, &oldFpeAction);

		// Add handler for SIGSEGV
		sa.sa_sigaction = &handleSegv;
		sigemptyset(&sa.sa_mask);
		sa.sa_flags = SA_RESTART | SA_SIGINFO;
		sigaction(SIGSEGV, &sa, &oldSegvAction);
	}

#endif

#ifdef WINDOWS

	static void initialize() {
		TODO(L"Implement me!");
	}

#endif

	static bool initialized = false;

	void setupSystemExceptions() {
		static util::Lock lock;
		util::Lock::L z(lock);

		if (initialized)
			return;
		initialized = true;

		initialize();
	}

}
