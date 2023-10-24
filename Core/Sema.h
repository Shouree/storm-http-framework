#pragma once
#include "Core/Object.h"

namespace storm {
	STORM_PKG(core.sync);

	/**
	 * A basic semaphore usable from whitin Storm. If possible, use a plain os::Lock or os::Sema as
	 * it does not require any separate allocations.
	 *
	 * This object slightly breaks the expected semantics of Storm as it does not make sense to copy
	 * a semaphore. Instead, all copies will refer to the same semaphore.
	 */
	class Sema : public Object {
		STORM_CLASS;
	public:
		// Create a semaphore initialized to 1.
		STORM_CTOR Sema();

		// Create a semaphore initialized to `count`.
		STORM_CTOR Sema(Nat count);

		// Copy.
		Sema(const Sema &o);

		// Destroy.
		~Sema();

		// Deep copy.
		void STORM_FN deepCopy(CloneEnv *e);

		// Increase the value in the semaphore by 1. May wake any waiting threads.
		void STORM_FN up();

		// Attempt to decrease the value in the semaphore by 1. Will wait in case the value would
		// become negative.
		void STORM_FN down();

	private:
		// Separate allocation for the actual lock. Allocated outside of the Gc since we do not
		// allow barriers or movement of the memory allocated for the lock.
		struct Data {
			size_t refs;
			os::Sema sema;

			Data(Nat count);
		};

		// Allocation. Not GC:d.
		Data *alloc;
	};

}
