#include "stdafx.h"
#include "ThreadPosix.h"
#include "Utils/Cache.h"

#if STORM_GC == STORM_GC_SMM && defined(POSIX)

#include "Utils.h"
#include <signal.h>

namespace storm {
	namespace smm {

		// Signals to use.
		static const int sigStop = SIGRTMIN;

		// Number of threads currently active. If >= 0, we have inserted a global signal handler.
		static size_t activeThreadCount = 0;

		// Old signal handler.
		static struct sigaction oldStopSig;

		// Per-thread information. We reuse this between multiple instances of the GC.
		static THREAD ThreadInfo *tlInfo = null;

		// Handle signals.
		static void onSigStop(int signal, siginfo_t *info, void *context) {
			// We need to save/restore errno.
			SaveLastError z;

			(void)signal;
			(void)info;

			if (tlInfo) {
				tlInfo->context = (ucontext_t *)context;
				tlInfo->onStop.up();
				tlInfo->onResume.down();
				clearLocalICache();
			}
		}


		OSThread::OSThread() : tid(pthread_self()), stopCount(0) {
			if (atomicIncrement(activeThreadCount) == 1) {
				// We're first.
				struct sigaction stopSig;
				stopSig.sa_sigaction = &onSigStop;
				stopSig.sa_flags = SA_SIGINFO | SA_RESTART;
				sigemptyset(&stopSig.sa_mask);
				sigaction(sigStop, &stopSig, &oldStopSig);
			}

			if (!tlInfo) {
				tlInfo = new ThreadInfo();
				tlInfo->self = &tlInfo;
			}

			info = tlInfo;
			atomicIncrement(info->refs);
		}

		OSThread::~OSThread() {
			if (atomicDecrement(info->refs) == 0) {
				*info->self = null;
				delete info;
			}

			if (atomicDecrement(activeThreadCount) == 0) {
				// We're last.
				sigaction(sigStop, &oldStopSig, NULL);
			}
		}

		void OSThread::requestStop() {
			if (stopCount++ == 0) {
				pthread_kill(tid, sigStop);
			}
			stopQueries++;
		}

		void OSThread::ensureStop() {
			assert(stopQueries > 0, L"ensureStop called without requestStop");
			--stopQueries;

			info->onStop.down();
		}

		void OSThread::start() {
			if (stopCount > 0) {
				if (--stopCount == 0) {
					info->onResume.up();
				}
			}
		}

	}
}

#endif
