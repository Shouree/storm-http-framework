#include "stdafx.h"
#include "VMPosix.h"

#if STORM_GC == STORM_GC_SMM && defined(POSIX)

#include "Utils/Cache.h"
#include "Utils.h"
#include "VMAlloc.h"
#include "Block.h"
#include <sys/mman.h>
#include <signal.h>

/**
 * The generic implementation for all Posix systems.
 *
 * Note: We can't use userfaultfd on Linux at the moment, since it does not support handling
 * notifications for write-protected pages, only missing pages.
 */
namespace storm {
	namespace smm {

		extern "C" void gc_panic_stacktrace();

		static const int protection = PROT_EXEC | PROT_READ | PROT_WRITE;
		static const int protectionWp = PROT_EXEC | PROT_READ;
		static const int flags = MAP_ANONYMOUS | MAP_PRIVATE;

		static struct sigaction oldAction;

		void VMPosix::sigsegv(int signal, siginfo_t *info, void *context) {
			// We need to save and restore errno.
			SaveLastError z;

			(void)signal;
			(void)context;

			// If we can do something about the error, we don't need to panic.
			if (VM::notifyWrite(info->si_addr)) {
				clearLocalICache();
				return;
			}

			// We don't know this memory. Trigger another signal to terminate us.
#ifdef DEBUG
			gc_panic_stacktrace();
#endif
			raise(SIGINT);
		}

		void VMPosix::initNotify() {
			// Insert the signal handler for SIGSEGV.
			struct sigaction action;
			action.sa_sigaction = &VMPosix::sigsegv;
			action.sa_flags = SA_SIGINFO | SA_RESTART;
			sigemptyset(&action.sa_mask);

			sigaction(SIGSEGV, &action, &oldAction);
		}

		void VMPosix::destroyNotify() {
			sigaction(SIGSEGV, &oldAction, NULL);
		}


		VMPosix *VMPosix::create(VMAlloc *alloc) {
			size_t pageSize = sysconf(_SC_PAGESIZE);
			return new VMPosix(alloc, pageSize);
		}

		VMPosix::VMPosix(VMAlloc *alloc, size_t pageSize) : VM(alloc, pageSize, pageSize) {}

		void *VMPosix::reserve(void *at, size_t size) {
			return mmap(at, size, PROT_NONE, flags, -1, 0);
		}

		void VMPosix::commit(void *at, size_t size) {
			mprotect(at, size, protection);
		}

		void VMPosix::decommit(void *at, size_t size) {
			// Re-map pages with PROT_NONE to tell the kernel we're no longer interested in this memory.
			void *result = mmap(at, size, PROT_NONE, flags | MAP_FIXED, -1, 0);
			if (result == MAP_FAILED)
				PLN(L"Decommit failed!");
		}

		void VMPosix::free(void *at, size_t size) {
			munmap(at, size);
		}

		void VMPosix::watchWrites(void *at, size_t size) {
			mprotect(at, size, protectionWp);
		}

		void VMPosix::stopWatchWrites(void *at, size_t size) {
			mprotect(at, size, protection);
		}

	}
}

#endif
