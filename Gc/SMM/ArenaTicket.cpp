#include "stdafx.h"
#include "ArenaTicket.h"

#if STORM_GC == STORM_GC_SMM

#include "Nonmoving.h"
#include "FinalizerPool.h"
#include "Thread.h"
#include "Utils/Cache.h"

namespace storm {
	namespace smm {

		LockTicket::LockTicket(Arena &owner) : owner(owner), locked(false) {
			lock();
		}

		LockTicket::~LockTicket() {
			unlock();
		}

		void LockTicket::lock() {
			owner.arenaLock.lock();
			dbg_assert(owner.entries == 0, L"Recursive arena entry!");
			owner.entries++;
			locked = true;
		}

		void LockTicket::unlock() {
			if (!locked)
				return;

			owner.entries--;
			owner.arenaLock.unlock();
			locked = false;
			clearLocalICache();
		}


		ArenaTicket::ArenaTicket(Arena &owner) : LockTicket(owner), gc(false), threads(false), objectsMoved(false) {}

		ArenaTicket::~ArenaTicket() {
			startThreads();
			unlock();
		}

		void ArenaTicket::stopThreads() {
			if (threads)
				return;

			size_t currentId = OSThread::currentId();

			InlineSet<Thread>::iterator begin = owner.threads.begin();
			InlineSet<Thread>::iterator end = owner.threads.end();

			// Request all other threads to stop.
			for (InlineSet<Thread>::iterator i = begin; i != end; ++i) {
				if (currentId == i->id())
					continue;

				i->requestStop();
			}

			// Then make sure they are stopped. This two-stage system makes it possible for multiple
			// stop "messages" to be in flight concurrently.
			for (InlineSet<Thread>::iterator i = begin; i != end; ++i) {
				if (currentId == i->id())
					continue;

				i->ensureStop();
			}

			threads = true;
		}

		void ArenaTicket::startThreads() {
			if (!threads)
				return;

			InlineSet<Thread>::iterator begin = owner.threads.begin();
			InlineSet<Thread>::iterator end = owner.threads.end();

			// Start all threads. Calling 'start' on a non-stopped thread is fine.
			for (InlineSet<Thread>::iterator i = begin; i != end; ++i) {
				i->start();
			}

			threads = false;
		}

		void ArenaTicket::scheduleCollection(Generation *gen) {
			triggered.add(gen->identifier);
		}

		bool ArenaTicket::suggestCollection(Generation *gen) {
			if (gc)
				return false;

			// We always deny suggestions when we're in ramp mode.
			if (atomicRead(owner.rampAttempts) > 0) {
				atomicOr(owner.rampAttempts, size_t(1) << size_t(gen->identifier));
				return false;
			}

			owner.collectI(*this, GenSet(gen->identifier));
			return true;
		}

		void ArenaTicket::finalize() {
			if (triggered.any()) {
				GenSet collected;
				GenSet old;

				do {
					// Don't collect the same generation twice!
					old = triggered;
					old.remove(collected);
					triggered.clear();

					if (old.any())
						collected.add(owner.collectI(*this, old));
				} while (old.any());
			}

			// Check if we need to update the history.
			if (objectsMoved)
				owner.history.step(*this);

			// Unlock and run finalizers!
			startThreads();
			unlock();

			{
				FinalizerContext context;
				// Note: Since references from finalizers keep nonmoving objects alive, but not the
				// other way around, we want to run finalizers for nonmoving objects first.
				owner.nonmoving().runFinalizers(context);
				owner.finalizers->finalize(context);
			}
		}


	}
}

#endif
