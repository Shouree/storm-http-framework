#pragma once
#include "Core/Object.h"

namespace storm {
	STORM_PKG(core.sync);

	/**
	 * A basic event usable from within Storm. If possible, use a plain os::Event as it does not
	 * require any separate allocations.
	 *
	 * The event is initially cleared, which means that any calls to `wait` will block until another
	 * thread calls `set`. The event is then in its signaled state, so any calls to `wait` will block
	 *
	 * This object slightly breaks the expected semantics of Storm as it does not make sense to copy
	 * a semaphore. Instead, all copies will refer to the same semaphore.
	 */
	class Event : public Object {
		STORM_CLASS;
	public:
		// Create an event. The created event is initially not set.
		STORM_CTOR Event();

		// Copy.
		Event(const Event &o);

		// Destroy.
		~Event();

		// Deep copy.
		void STORM_FN deepCopy(CloneEnv *e);

		// Set the event. This causes all threads currently waiting to continue executing, and all
		// future calls to `wait` to return immediately.
		void STORM_FN set();

		// Clear the event. This causes future threads to `wait` to cause threads to wait for the
		// event to become set again. Note that calling `clear` while other threads might call
		// `wait` causes a race condition, as there is no guarantee in what order the two operations
		// will complete. As such, some caution is required when using `clear`.
		void STORM_FN clear();

		// Wait for the event to become signaled, if it is not already.
		void STORM_FN wait();

		// Check if the condition is set. While this function is thread safe, it is generally risky
		// to inspect the state of an event and later act upon it. After checking the state, there
		// is generally no guarantee that the state remains the same immediately after checking it.
		Bool STORM_FN isSet();

	private:
		// Separate allocation for the actual event. Allocated outside of the Gc since we do not
		// allow barriers or movement of the memory allocated for the event.
		struct Data {
			size_t refs;
			os::Event event;

			Data();
		};

		// Allocation. Not GC:d.
		Data *alloc;
	};

}
