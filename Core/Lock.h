#pragma once
#include "Core/Object.h"
#include "OS/Sync.h"

namespace storm {
	STORM_PKG(core.sync);

	/**
	 * A recursive lock usable from within Storm. If possible, use a plain os::Lock or os::Sema from
	 * C++ as it does not require any separate allocations.
	 *
	 * This object slightly breaks the expected semantics of Storm as it does not make sense to copy
	 * a lock. Instead, all copies will refer to the same semaphore.
	 *
	 * Note: usually it is not required to use locks within Storm as the threading model takes care
	 * of most cases where locks would be needed.
	 */
	class Lock : public Object {
		STORM_CLASS;
	public:
		// Create.
		STORM_CTOR Lock();
		Lock(const Lock &o);

		// Destroy.
		~Lock();

		// Deep copy.
		void STORM_FN deepCopy(CloneEnv *e);

		// Lock guard for C++ and Storm.
		class Guard {
			STORM_VALUE;
		public:
			STORM_CTOR Guard(Lock *o);
			~Guard();
		private:
			Lock *lock;

			// Actually implemented, as they might be called from Storm.
			Guard(const Guard &o);
			Guard &operator =(const Guard &o);
		};

	private:
		struct Data {
			size_t refs;
			size_t owner;
			size_t recursion;
			os::Sema sema;

			Data();
		};

		Data *alloc;

		// Lock and unlock.
		void lock();
		void unlock();
	};

}
