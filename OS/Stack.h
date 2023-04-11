#pragma once
#include "InlineSet.h"

namespace os {

	/**
	 * Stack description used by the UThread stack scheduler.
	 *
	 * This class may refer to a stack that was allocated by us, or to a stack that was allocated by
	 * the OS. Either way, the destructor handles deallocation properly.
	 *
	 * Note that a stack may be in two states. Either it is running or it is sleeping. If it is
	 * sleeping, the 'desc' member is valid, and the old stack pointer can be retrieved in that
	 * manner. Otherwise, the stack pointer must be found by examining all running threads
	 * (typically, the system knows which thread a particular stack belongs to, so it does not need
	 * to search for it).
	 *
	 * When a thread has been allocated, a sample "desc" is initialized. This description is
	 * located in the "cold" end of the stack, i.e. the end of the stack that will not be
	 * immediately overwritten. The "low" and "high" values are both initialized to the "hot" end of
	 * the stack, i.e. where values are to be pushed.
	 */
	class Stack : public SetMember<Stack> {
	public:
		// Allocate a stack with the given size.
		explicit Stack(size_t size);

		// Create a stack that represents a OS-allocated stack. It is assumed to be running at the
		// moment. The parameter provided is the address of the "base" of the stack, i.e. somewhere
		// near the bottom of the stack. Typically the address of a local variable near "main".
		explicit Stack(void *base);

		// Destroy.
		~Stack();

		// Description.
		struct Desc {
			// Information about the current stack. 'high' is the currently highest used address
			// (inclusive) and 'low' is the lowest used address (exclusive).
			void *low;
			void *dummy;
			void *high;
		};

		// Current stack description. If null, then this UThread is currently running, and the
		// current CPU state describes the stack of that thread. "desc" always refers to somewhere
		// on the stack this represents.
		// Note: Assumed to be the first member of the class, except for the pointers in SetMember.
		Desc *desc;

		// Get the limit of the stack, i.e. the cold end of the stack.
		inline void *limit() const {
			return alloc;
		}

		// Get the base of the stack, i.e. the hot end of the stack, which is typically always used.
		inline void *base() const {
			return (byte *)alloc + size;
		}

		// Is this thread participating in a detour? Updated atomically.
		// If set, this stack is not considered to belong to the thread for which the thread's set
		// contain this stack. Instead, it it considered to be a member of the thread which has a
		// stack that points to this stack from its 'detourTo' member (either directly or
		// indirectly). This is done to allow atomic migrations from one thread to another. However,
		// it can cause a stack to be scanned twice if stack scanning happens at an unfortunate time.
		nat detourActive;

		// Which thread is currently running instead of this thread?
		// Note: Assumed to be here by StackCall.h
		Stack *detourTo;

		// Clear this stack. I.e. re-initialize an empty desc on top of it. Assumes it was
		// allocated, and not an OS stack.
		void clear();

		// Get the low and high address of the allocated memory (if any). These are independent of
		// the current CPU architecture.
		inline void *low() const { return alloc; }
		inline void *high() const { return (byte *)alloc + size; }

		// Check if this stack was allocated by the Stack class.
		inline bool allocated() const { return size > 0; }

	private:
		// The low address of the stack. If we represent an OS allocated stack, this is the limit.
		void *alloc;

		// The size of the stack. If we represent an OS allocated stack, this is zero.
		size_t size;

		// Allocate an actual stack into "alloc" and "size".
		void allocate(size_t size);

		// Free the stack in "alloc" and "size".
		void free();

		// Initialize "desc", assuming we have allocated a stack.
		void initDesc();
	};

}
