#include "stdafx.h"
#include "SystemException.h"
#include "Utils/Lock.h"
#include "Gc/DwarfTable.h"
#include "Core/Exception.h"
#include "Code/Arena.h"
#include "Code/Binary.h"

#ifdef POSIX
#define XOPEN
#include <signal.h>
#include <ucontext.h>
#endif

namespace storm {

	static Engine *findEngine(const void *pc) {
		FDE *fde = dwarfTable().find(pc);
		if (!fde)
			return null;

		code::Binary *b = code::codeBinary(fde->codeStart());
		if (!b)
			return null;

		return &b->engine();
	}

#ifdef POSIX

#ifdef X64
	static const void *instructionPtr(const ucontext_t *ctx) {
		return (const void *)ctx->uc_mcontext.gregs[REG_RIP];
	}
#endif

#ifdef X86
	static const void *instructionPtr(const ucontext_t *ctx) {
		return (const void *)ctx->uc_mcontext.gregs[REG_EIP];
	}
#endif

#ifdef ARM64
	static const void *instructionPtr(const ucontext_t *ctx) {
		return (const void *)ctx->uc_mcontext.pc;
	}
#endif

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
		// extract an Engine object to instantiate the exception. Note: the si_addr member here is
		// the address of the faulting access, not the address of the faulting instruction. To get
		// the faulting instruction, we need to look inside 'context'.
		Engine *e = findEngine(instructionPtr((ucontext_t *)context));
		if (!e)
			e = runtime::someEngineUnsafe();

		if (e) {
			switch (info->si_code) {
			case SEGV_MAPERR:
				throw new (*e) MemoryAccessError(Word(info->si_addr), false);
			case SEGV_ACCERR:
				throw new (*e) MemoryAccessError(Word(info->si_addr), true);
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
