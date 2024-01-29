#include "stdafx.h"
#include "StackMirror.h"
#include "Engine.h"

namespace storm {

	StackMirror::StackMirror(os::UThreadData *thread) : thread(thread), root(null), stackContents(null), stackCount(0) {
		if (thread)
			thread->addRef();
	}

	StackMirror::StackMirror(const StackMirror &o) : thread(o.thread), root(null), stackContents(null), stackCount(0) {
		if (o.thread)
			thread->addRef();

		allocContents(o.stackCount);
		for (size_t i = 0; i < o.stackCount; i++)
			stackContents[i] = o.stackContents[i];
	}

	StackMirror::~StackMirror() {
		clear();
	}

	StackMirror &StackMirror::operator =(const StackMirror &o) {
		if (&o == this)
			return *this;

		clear();

		thread = o.thread;
		if (thread)
			thread->addRef();

		allocContents(o.stackCount);
		for (size_t i = 0; i < o.stackCount; i++)
			stackContents[i] = o.stackContents[i];

		return *this;
	}

	void StackMirror::clear() {
		if (thread)
			thread->release();
		thread = null;

		freeContents();
	}

	void StackMirror::allocContents(size_t count) {
		stackCount = count;
		stackContents = (size_t *)malloc(count * sizeof(size_t));
		memset(stackContents, 0, count * sizeof(size_t));
		root = engine().gc.createRoot(stackContents, count, true);
	}

	void StackMirror::freeContents() {
		if (root)
			Gc::destroyRoot(root);
		root = null;

		if (stackContents)
			free(stackContents);
		stackContents = null;
		stackCount = 0;
	}

	StackMirror *StackMirror::save(EnginePtr e, Word threadId) {
		os::UThreadData *data = reinterpret_cast<os::UThreadData *>(static_cast<size_t>(threadId));
		if (!data)
			return null;

		// If the desc does not exist, then it is currently running, and we can not copy it.
		const os::Stack &stack = data->stack;
		if (!stack.desc)
			return null;

		// We more or less assume that the desc is at the top of the stack. This is always the case,
		// except maybe when the thread was just created, or when a detour is set up.
		if (stack.desc->low != stack.desc)
			return null;

		StackMirror *mirror = new (e.v) StackMirror(data);

		size_t *low = (size_t *)stack.desc->low;
		size_t toCopy = size_t(stack.base()) - size_t(low);
		toCopy /= sizeof(size_t);

		mirror->allocContents(toCopy);
		for (size_t i = 0; i < toCopy; i++) {
			mirror->stackContents[i] = low[i];
		}

		return mirror;
	}

	Bool StackMirror::restore() {
		if (!thread || !stackContents)
			return false;

		os::Stack &stack = thread->stack;
		if (!stack.desc)
			return false;

		size_t *base = (size_t *)stack.base();
		base -= stackCount;

		// Note: We are storing the stack contents in a pinned set already, so it is fine for us to
		// copy first and set the 'desc' later. No objects will be missed anyway. Otherwise, we
		// would have to copy the StackDesc first, then update 'desc' and then copy the rest of the data.
		for (size_t i = 0; i < stackCount; i++)
			base[i] = stackContents[i];

		stack.desc = (os::Stack::Desc *)base;

		// Check if the thread is scheduled anywhere. If not, put it in the ready queue now.
		if (!thread->next) {
			thread->owner()->wake(thread);
		}

		return true;
	}

}
