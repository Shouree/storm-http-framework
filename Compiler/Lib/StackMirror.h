#pragma once
#include "Core/Object.h"
#include "Core/EnginePtr.h"
#include "OS/UThread.h"
#include "Gc/Gc.h"

namespace storm {
	STORM_PKG(core.unsafe);

	/**
	 * Stores a copy of a UThread's stack alongside a reference to the thread itself.
	 *
	 * This makes it possible to restore the state of that UThread to the state when it was saved.
	 * This makes it possible to implement continuations and effects.
	 */
	class StackMirror : public Object {
		STORM_CLASS;
	public:
		// Copy. Note: DeepCopy is not needed.
		StackMirror(const StackMirror &o);

		// Destroy, free memory.
		~StackMirror();

		// Assign.
		StackMirror &operator =(const StackMirror &o);

		// Early destruction of the stack mirror.
		void STORM_FN clear();

		// Create a copy of the current state of an UThread. 'threadId' is expected to be the value
		// returned from 'currentUThread', and the thread is expected to be alive and currently not
		// running. Ideally, the thread should be waiting for some synchronization event, like a
		// semaphore.
		static StackMirror *STORM_FN save(EnginePtr e, Word threadId);

		// Restore the stack to the previously saved UThread. Assumes that the thread is not
		// currently running. If the thread is not currently present in a ready-queue, then the
		// thread is immediately placed there.
		Bool STORM_FN restore();

	private:
		// Create.
		StackMirror(os::UThreadData *thread);

		// UThread that the stack was copied from.
		// We hold a reference to this thread.
		UNKNOWN(PTR_NOGC) os::UThreadData *thread;

		// Registered GC root.
		UNKNOWN(PTR_NOGC) Gc::Root *root;

		// Copied stack contents, from the base of the stack.
		UNKNOWN(PTR_NOGC) size_t *stackContents;

		// Number of words in the stack contents.
		size_t stackCount;

		// Allocate memory for 'stackContents'. Assumes that it was previously empty.
		void allocContents(size_t count);

		// Free memory in 'stackContents'.
		void freeContents();
	};

}
