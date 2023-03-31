#include "stdafx.h"
#include "UThread.h"
#include "Thread.h"
#include "FnCall.h"
#include "Shared.h"
#include "Sync.h"
#include "Utils/Bitwise.h"
#include "Utils/Lock.h"
#include <limits>

#ifdef POSIX
#include <sys/mman.h>
#endif

namespace os {

	/**
	 * Forward declare machine specific functions at the bottom.
	 */

	typedef Stack::Desc StackDesc;

	// Stack size. (we need about 30k on Windows to do cout).
	// 40k is too small to run the compiler well.
    // TODO: Make compiler UThreads larger somehow? Maybe smaller stacks are enough in release builds...
	// TOOD: On Windows, we want allocations to be 64k-aligned, as that is the allocation granularity there.
	static nat stackSize = 400 * 1024;

	// Switch the currently running threads. *oldEsp is set to the old esp.
	// This returns as another thread, which may mean that it returns to the
	// beginning of another function in the case of newly started UThreads.
	extern "C" void doSwitch(StackDesc **newEsp, StackDesc **oldEsp);

	// Switch back to the previously running UThread, allocating an additional return address on the
	// stack and remembering the location of that stack so that it can be altered later.
	// Returns the function that shall be called, since there may be variants of the function.
	static const void *endDetourFn(bool member);

	// Get a platform-dependent highly accurate timestamp in some unit. This should be monotonically
	// increasing.
	static int64 timestamp();

	// Convert a millisecond interval into the units of the high frequency unit.
	static int64 msInTimestamp(nat ms);

	// Check how much time remains until a given timestamp (in ms).
	static nat remainingMs(int64 timestamp);


	/**
	 * UThread.
	 */

	static ThreadData *threadData(const Thread *thread) {
		if (thread && *thread != Thread::invalid)
			return thread->threadData();
		return Thread::current().threadData();
	}

	// Insert on a specific thread. Makes sure to keep 'data' alive until the function has returned at least.
	UThread UThread::insert(UThreadData *data, ThreadData *on) {
		// We need to take our reference here, from the moment we call 'insert', 'data' may be deleted otherwise!
		UThread result(data);

		on->uState.insert(data);

		return result;
	}

	// End the current thread.
	static void exitUThread() {
		UThreadState::current()->exit();
	}

	const UThread UThread::invalid(null);

	UThread::UThread(UThreadData *data) : data(data) {
		if (data)
			data->addRef();
	}

	UThread::UThread(const UThread &o) : data(o.data) {
		if (data)
			data->addRef();
	}

	UThread &UThread::operator =(const UThread &o) {
		if (data)
			data->release();
		data = o.data;
		if (data)
			data->addRef();
		return *this;
	}

	UThread::~UThread() {
		if (data)
			data->release();
	}

	bool UThread::leave() {
		return UThreadState::current()->leave();
	}

	bool UThread::anySleeping() {
		return UThreadState::current()->anySleeping();
	}

	void UThread::sleep(nat ms) {
		UThreadState::current()->sleep(ms);
	}

	bool UThread::any() {
		return UThreadState::current()->any();
	}

	UThread UThread::current() {
		return UThread(UThreadState::current()->runningThread());
	}

	static void onUncaughtException() {
		try {
			throw;
		} catch (const PtrThrowable *e) {
			PLN("Uncaught exception from UThread: " << e->toCStr());
		} catch (const ::Exception &e) {
			PLN("Uncaught exception from UThread: " << e);
		} catch (...) {
			PLN("Uncaught exception from UThread: <unknown>");
		}
	}

	static void spawnFn(util::Fn<void, void> *fn) {
		try {
			(*fn)();
		} catch (...) {
			onUncaughtException();
		}

		delete fn;
		exitUThread();
	}

	UThread UThread::spawn(const util::Fn<void, void> &fn, const Thread *on) {
		ThreadData *thread = os::threadData(on);
		UThreadData *t = UThreadData::create(&thread->uState);

		t->pushContext(address(&spawnFn), new util::Fn<void, void>(fn));

		return insert(t, thread);
	}

	struct SpawnParams {
		bool memberFn;
		void *first;
		void **params;
		CallThunk thunk;

		// Only valid when using futures.
		void *target;
		FutureBase *future;
	};

	static void spawnCall(SpawnParams *params) {
		try {
			(*params->thunk)(endDetourFn(params->memberFn), params->memberFn, params->params, params->first, null);
		} catch (...) {
			onUncaughtException();
		}

		// Terminate.
		exitUThread();
	}

	static void spawnCallFuture(SpawnParams *params) {
		// We need to save a local copy of 'future' since 'params' reside on the stack of the
		// caller, which will continue as soon as we try to call the function.
		FutureBase *future = params->future;

		try {
			(*params->thunk)(endDetourFn(params->memberFn), params->memberFn, params->params, params->first, params->target);
			future->posted();
		} catch (...) {
			future->error();
		}

		// Terminate.
		exitUThread();
	}

	typedef void (*SpawnFn)(SpawnParams *params);

	static UThreadData *spawnHelper(SpawnFn spawn, const void *fn, ThreadData *thread, SpawnParams *params) {
		// Create a new UThread on the proper thread.
		UThreadData *t = UThreadData::create(&thread->uState);

		// Set up the thread for calling 'spawnCall'.
		t->pushContext((const void *)spawn, params);

		// Call the newly created UThread on this thread, and make sure to return directly here later.
		UThreadState *current = UThreadState::current();
		void *returnAddr = current->startDetour(t);

		// Make sure we return to the function we were supposed to call.
		*(const void **)returnAddr = fn;

		// Done!
		return t;
	}

	UThread UThread::spawnRaw(const void *fn, bool memberFn, void *first, const FnCallRaw &call, const Thread *on) {
		ThreadData *thread = os::threadData(on);

		SpawnParams params = {
			memberFn,
			first,
			call.params(),
			call.thunk,
			null,
			null
		};
		UThreadData *t = spawnHelper(&spawnCall, fn, thread, &params);
		return insert(t, thread);
	}

	UThread UThread::spawnRaw(const void *fn, bool memberFn, void *first, const FnCallRaw &call, FutureBase &result,
							void *target, const Thread *on) {
		ThreadData *thread = os::threadData(on);

		SpawnParams params = {
			memberFn,
			first,
			call.params(),
			call.thunk,
			target,
			&result
		};
		UThreadData *t = spawnHelper(&spawnCallFuture, fn, thread, &params);
		return insert(t, thread);
	}

	static void detourMain(const util::Fn<void, void> *fn) {
		try {
			(*fn)();
		} catch (...) {
			onUncaughtException();
		}

		// End the detour.
		typedef void (*EndFn)();
		EndFn endFn = (EndFn)(endDetourFn(false));
		(*endFn)();
	}

	bool UThread::detour(const util::Fn<void, void> &fn) {
		UThread current = UThread::current();

		// Current thread?
		if (data == current.data)
			return false;

		// Same Thread?
		if (data->owner != current.data->owner)
			return false;

		// Running?
		if (!data->stack.desc)
			return false;

		// Should be safe for now.

		// Add the new context on top of the stack frame containing the old resume point.
		StackDesc *old = data->pushSubContext(address(&detourMain), (void *)&fn);

		// Need to reset the owner to make the detour system happy.
		data->owner = null;
		current.data->owner->startDetour(data);
		data->owner = current.data->owner;

		// Now, the detour has finished execution. Restore the old state.
		data->restoreContext(old);

		return true;
	}


	/**
	 * UThread data.
	 */

	UThreadData::UThreadData(UThreadState *state, size_t size)
		: references(0), next(null), owner(null), stack(size),
		  detourOrigin(null), detourResult(null) {

		// Notify the GC that we exist and may contain interesting data.
		// Note: This UThread must be scannable by this point!
		state->newStack(this);
	}

	UThreadData::UThreadData(UThreadState *state, void *limit)
		: references(0), next(null), owner(null), stack(limit),
		  detourOrigin(null), detourResult(null) {

		// Notify the GC that we exist and may contain interesting data.
		// Note: This UThread must be scannable by this point!
		state->newStack(this);
	}

	UThreadData *UThreadData::createFirst(UThreadState *thread, void *base) {
		return new UThreadData(thread, base);
	}

	UThreadData *UThreadData::create(UThreadState *thread) {
		return new UThreadData(thread, os::stackSize);
	}

	UThreadData::~UThreadData() {}

	void UThreadData::switchTo(UThreadData *to) {
		doSwitch(&to->stack.desc, &stack.desc);
	}

	/**
	 * UThread state.
	 */

	UThreadState::UThreadState(ThreadData *owner, void *stackBase) : owner(owner) {
		currentUThreadState(this);

		running = UThreadData::createFirst(this, stackBase);
		running->owner = this;
		running->addRef();

		aliveCount = 1;
	}

	UThreadState::~UThreadState() {
		currentUThreadState(null);
		if (running) {
			stacks.erase(&running->stack);
			running->release();
			atomicDecrement(aliveCount);
		}

		dbg_assert(aliveCount == 0, L"An OS thread tried to terminate before all its UThreads terminated.");
		dbg_assert(stacks.empty(), L"Consistency issue. aliveCount says zero, but we still have stacks we can scan!");
	}

	void UThreadState::newStack(UThreadData *v) {
		util::Lock::L z(lock);
		stacks.insert(&v->stack);
	}

	vector<UThread> UThreadState::idleThreads() {
		vector<UThread> result;

		// Note: The lock mainly protects against insertions from other threads. We don't have to
		// worry about deletions, as those are always performed by this OS thread.
		util::Lock::L z(lock);
		for (InlineSet<Stack>::iterator at = stacks.begin(), end = stacks.end(); at != end; ++at) {
			if (!at->desc)
				continue;

			result.push_back(UThread(UThreadData::fromStack(at)));
		}

		return result;
	}

	UThreadState *UThreadState::current() {
		UThreadState *s = currentUThreadState();
		if (!s) {
			Thread::current();
			s = currentUThreadState();
			assert(s);
		}
		return s;
	}

	bool UThreadState::any() {
		return atomicRead(aliveCount) != 1;
	}

	bool UThreadState::leave() {
		reap();

		UThreadData *prev = running;
		{
			util::Lock::L z(lock);
			UThreadData *next = ready.pop();
			if (!next)
				return false;
			ready.push(running);
			running = next;
		}

		// Any IO messages for this thread?
		owner->checkIo();

		// NOTE: This does not always return directly. Consider this when writing code after this statement.
		prev->switchTo(running);

		reap();
		return true;
	}

	bool UThreadState::anySleeping() {
		return sleeping.any();
	}

	void UThreadState::sleep(nat ms) {
		struct D : public SleepData {
			D(int64 until) : SleepData(until), sema(0) {}
			os::Sema sema;
			virtual void signal() {
				sema.up();
			}
		};

		D done(timestamp() + msInTimestamp(ms));
		sleeping.push(&done);
		done.sema.down();
	}

	bool UThreadState::nextWake(nat &time) {
		SleepData *first = sleeping.peek();
		if (!first)
			return false;

		time = remainingMs(first->until);
		return true;
	}

	void UThreadState::wakeThreads() {
		wakeThreads(timestamp());
	}

	void UThreadState::wakeThreads(int64 time) {
		while (sleeping.any()) {
			SleepData *first = sleeping.peek();
			if (time >= first->until) {
				first->signal();
				sleeping.pop();
			} else {
				break;
			}
		}
	}

	void UThreadState::exit() {
		// Do we have something to exit to?
		UThreadData *prev = running;
		UThreadData *next = null;

		while (true) {
			{
				util::Lock::L z(lock);
				next = ready.pop();
			}

			if (next)
				break;

			// Wait for the signal.
			owner->waitForWork();
		}

		// Now, we have something else to do!
		{
			util::Lock::L z(lock);
			stacks.erase(&prev->stack);
		}
		atomicDecrement(aliveCount);
		exited.push(prev);
		running = next;
		prev->switchTo(running);

		// Should not return.
		assert(false);
	}

	void UThreadState::insert(UThreadData *data) {
		data->owner = this;
		dbg_assert(stacks.contains(&data->stack), L"WRONG THREAD");
		atomicIncrement(aliveCount);

		{
			util::Lock::L z(lock);
			ready.push(data);
			data->addRef();
		}

		// Notify that we need to wake up now!
		owner->reportWake();
	}

	void UThreadState::wait() {
		UThreadData *prev = running;
		UThreadData *next = null;

		while (true) {
			{
				util::Lock::L z(lock);
				next = ready.pop();
			}

			if (next == prev) {
				// May happen if we are the only UThread running and someone managed to wake the
				// thread before we had time to sleep.
				break;
			} else if (next) {
				running = next;
				prev->switchTo(running);
				break;
			}

			// Nothing to do in the meantime...
			owner->waitForWork();
		}

		reap();
	}

	void UThreadState::wake(UThreadData *data) {
		{
			util::Lock::L z(lock);
			ready.push(data);
		}

		// Make sure we're not waiting for something that has already happened.
		owner->reportWake();
	}

	void UThreadState::reap() {
		while (UThreadData *d = exited.pop())
			d->release();

		wakeThreads(timestamp());
		owner->checkIo();
	}

	void *UThreadState::startDetour(UThreadData *to) {
		assert(to->owner == null, L"The UThread is already associated with a thread, can not use it for detour.");

		UThreadData *prev = running;
		Stack *prevStack = &prev->stack;

		// Since the UThread might be in the process of a stack call, we cannot be sure that
		// "detourTo" is empty. If it contains something, we need to traverse the list of "detourTo"
		// to find the last element and add the detour there. Otherwise, we will trash the scanning
		// due to resetting "detourTo" too early.
		while (Stack *stack = atomicRead(prevStack->detourTo)) {
			prevStack = stack;
		}

		// Remember the currently running UThread so that we can go back to it.
		to->owner = this;
		to->detourOrigin = prev;

		// Adopt the new UThread, so that it is scanned as if it belonged to our thread.
		// Note: it is important to do this in the proper order, otherwise there will be a
		// small window where the UThread is not scanned at all.
		atomicWrite(prevStack->detourTo, &to->stack);
		atomicWrite(to->stack.detourActive, 1);

		// Switch threads.
		running = to;
		prev->switchTo(running);

		// Since we returned here from 'switchTo', we know that 'endDetour' was called. Thus, we have
		// a result inside 'to', and we shall release our temporary adoption of the UThread now.
		// Note: We can not do that inside 'endDetour' since the other thread is currently
		// running. If we would alter 'detourTo' or 'detourActive' on a running thread, it is
		// possible that the stack scanning will think the thread is active for the wrong OS thread,
		// which would cause everything to crash due to the mismatch of stack pointers.
		atomicWrite(to->stack.detourActive, 0);
		atomicWrite(prevStack->detourTo, (Stack *)null);

		// Since we returned here from 'switchTo', we know that 'endDetour' has set the result for us!
		return to->detourResult;
	}

	void UThreadState::endDetour(void *result) {
		assert(running->detourOrigin, L"No active detour.");

		// Find the thread to run instead.
		UThreadData *prev = running;
		running = prev->detourOrigin;

		// Release the currently running UThread from our grip.
		prev->detourOrigin = null;
		prev->detourResult = result;
		prev->owner = null;

		// Switch threads.
		prev->switchTo(running);

		// When we're back, we're in a different thread. Thus, call "reap" to clean up lingering threads.
		// Note that we cannot re-use UThreadState::current here, as we're possibly in a different thread now!
		UThreadState::current()->reap();
	}


	/**
	 * OS-specific code.
	 */

#if defined(WINDOWS)

	static int64 timestamp() {
		LARGE_INTEGER v;
		QueryPerformanceCounter(&v);
		return v.QuadPart;
	}

	static int64 msInTimestamp(nat ms) {
		LARGE_INTEGER v;
		QueryPerformanceFrequency(&v);
		return (v.QuadPart * ms) / 1000;
	}

	static nat remainingMs(int64 target) {
		int64 now = timestamp();
		if (target <= now)
			return 0;

		LARGE_INTEGER v;
		QueryPerformanceFrequency(&v);
		return nat((1000 * (target - now)) / v.QuadPart);
	}

#elif defined(POSIX)

	static int64 timestamp() {
		struct timespec time = {0, 0};
		clock_gettime(CLOCK_MONOTONIC, &time);

		int64 r = time.tv_sec;
		r *= 1000 * 1000;
		r += time.tv_nsec / 1000;
		return r;
	}

	static int64 msInTimestamp(nat ms) {
		return int64(ms * 1000);
	}

	static nat remainingMs(int64 target) {
		int64 now = timestamp();
		if (target <= now)
			return 0;

		int64 remaining = target - now;
		return nat(remaining / 1000);
	}

#endif


	/**
	 * Machine specific code.
	 */

#if defined(X86)

	static void *&stackPtr(Stack &stack) {
		return stack.desc->low;
	}

	void UThreadData::push(void *v) {
		void **esp = (void **)stackPtr(stack);
		*--esp = v;
		stackPtr(stack) = esp;
	}

	void UThreadData::push(uintptr_t v) {
		void **esp = (void **)stackPtr(stack);
		*--esp = (void *)v;
		stackPtr(stack) = esp;
	}

	void UThreadData::push(intptr_t v) {
		void **esp = (void **)stackPtr(stack);
		*--esp = (void *)v;
		stackPtr(stack) = esp;
	}

	void UThreadData::pushContext(const void *fn) {
		push((void *)fn); // return to
		push(0); // ebp
		push(0); // ebx
		push(0); // esi
		push(0); // edi
		push(-1); // seh (end of list is -1)
		push(stack.desc->high);
		push(stack.limit());
		push(stack.desc->low); // current stack pointer (approximate, this is enough)

		// Set the 'desc' of 'stack' to the actual stack pointer, so that we can run code on this stack.
		atomicWrite(stack.desc, (StackDesc *)(stack.desc->low));
	}

	void UThreadData::pushContext(const void *fn, void *param) {
		push(param);
		push(null); // return address used for 'fn'.
		pushContext(fn);
	}

	StackDesc *UThreadData::pushSubContext(const void *fn, void *param) {
		StackDesc *old = stack.desc;

		// Extract things from the old context.
		size_t *oldStack = (size_t *)old;
		size_t limit = oldStack[1];
		size_t high = oldStack[2];
		size_t seh = oldStack[3];
		size_t edi = oldStack[4];
		size_t esi = oldStack[5];
		size_t ebx = oldStack[6];
		size_t ebp = oldStack[7];
		size_t eip = oldStack[8];

		// Create a new context. Note: We can't use 'push' since we don't want to destroy the old context.
		size_t *newStack = oldStack - 11;

		// Parameters to the called function.
		newStack[10] = size_t(param);
		newStack[9] = size_t(eip); // return address so that stack traces work even though we don't
								   // assume we will ever return.

		// New context.
		newStack[8] = size_t(fn);
		newStack[7] = size_t(ebp);
		newStack[6] = size_t(ebx);
		newStack[5] = size_t(esi);
		newStack[4] = size_t(edi);
		newStack[3] = size_t(seh);
		newStack[2] = size_t(high);
		newStack[1] = size_t(limit);
		newStack[0] = size_t(stack.desc->low);

		// Set the 'desc' of 'stack' to the actual stack pointer so that we can run code!
		atomicWrite(stack.desc, (StackDesc *)(newStack));

		return old;
	}

	void UThreadData::restoreContext(StackDesc *desc) {
		atomicWrite(stack.desc, desc);
	}

#pragma warning (disable: 4733)
	// Note: we need to keep fs:[4] and fs:[8] updated (stack begin and end) updated
	// for exceptions to work in our threads. These could be saved through the 'stack'
	// member in the UThread itself, but this is much easier and only uses 8 bytes on the
	// stack anyway.
	NAKED void doSwitch(StackDesc **newDesc, StackDesc **oldDesc) {
		__asm {
			// Prolog. Bonus: saves ebp!
			push ebp;
			mov ebp, esp;

			// Store parameters.
			mov eax, oldDesc;
			mov ecx, newDesc;
			mov edx, newDesc; // Trash edx so that the compiler do not accidentally assume it is untouched.

			// Store state.
			push ebx;
			push esi;
			push edi;
			push dword ptr fs:[0];
			push dword ptr fs:[4];
			push dword ptr fs:[8];

			// Current stack pointer, overlaps with 'high' in StackDesc.
			lea ebx, [esp-4];
			push ebx;

			// Report the state for the old thread. This makes it possible for the GC to scan that properly.
			mov [eax], esp;

			// Switch to the new stack.
			mov esp, [ecx];

			// Restore state.
			add esp, 4; // skip 'high'.
			pop dword ptr fs:[8];
			pop dword ptr fs:[4];
			pop dword ptr fs:[0];
			pop edi;
			pop esi;
			pop ebx;

			// Clear the new thread's description. This means that the GC will start scanning the
			// stack based on esp rather than what is in the StackDesc.
			mov DWORD PTR [ecx], 0;

			// Epilog. Bonus: restores ebp!
			pop ebp;
			ret;
		}
	}

	void doEndDetour2() {
		// Figure out the return address from the previous function.
		void *retAddr = null;
		__asm {
			lea eax, [ebp+4];
			mov retAddr, eax;
		}

		// Then call 'endDetour' of the current UThread.
		UThreadState::current()->endDetour(retAddr);
	}

	NAKED void doEndDetour() {
		// 'allocate' an extra return address by simply calling yet another function. This return
		// address can be modified to simulate a direct jump to another function with the same
		// parameters that were passed to this function.
		__asm {
			call doEndDetour2;
			ret; // probably not reached, but better safe than sorry!
		}
	}

	static const void *endDetourFn(bool member) {
		return address(&doEndDetour);
	}

#elif defined(GCC) && defined(X64)

	static void *&stackPtr(Stack &stack) {
		return stack.desc->low;
	}

	void UThreadData::push(void *v) {
		void **esp = (void **)stackPtr(stack);
		*--esp = v;
		stackPtr(stack) = esp;
	}

	void UThreadData::push(uintptr_t v) {
		void **esp = (void **)stackPtr(stack);
		*--esp = (void *)v;
		stackPtr(stack) = esp;
	}

	void UThreadData::push(intptr_t v) {
		void **esp = (void **)stackPtr(stack);
		*--esp = (void *)v;
		stackPtr(stack) = esp;
	}

	void UThreadData::pushContext(const void *fn) {
		pushContext(fn, null);
	}

	void UThreadData::pushContext(const void *fn, void *param) {
		// Previous function's return to. Needed to align the stack properly.  Note: we can not push
		// an address to the start of a function here since that would break stack traces when using
		// DWARF. It is possible we could get away by using a function with no DWARF data.
		push((void *)0);

		push((void *)fn); // return to
		push((void *)0); // rbp
		push(param); // rdi
		push((void *)0); // rbx
		push((void *)0); // r12
		push((void *)0); // r13
		push((void *)0); // r14
		push((void *)0); // r15

		// Dummy value to make it possible to chain contexts.
		push((void *)0);

		push(stack.desc->high);
		push(stack.desc->dummy);
		push(stack.desc->low);

		// Set the 'desc' of 'stack' to the actual stack pointer, so that we can run code on this stack.
		atomicWrite(stack.desc, (StackDesc *)(stack.desc->low));
	}

	// Logical location where thread switches are made. Used to get good backtraces.
	// Note: This is a label in asm, so the interesting thing about this is its address.
	extern "C" char doSwitchReturnLoc;

	StackDesc *UThreadData::pushSubContext(const void *fn, void *param) {
		StackDesc *old = stack.desc;

		// Extract things from the old context.
		size_t *oldStack = (size_t *)old;
		size_t low = oldStack[0];
		size_t dummy = oldStack[1];
		size_t high = oldStack[2];
		size_t r15 = oldStack[4];
		size_t r14 = oldStack[5];
		size_t r13 = oldStack[6];
		size_t r12 = oldStack[7];
		size_t rbx = oldStack[8];
		// size_t rdi = oldStack[9]; // Not needed.
		size_t rbp = oldStack[10];
		// size_t rip = oldStack[11]; // Not needed.

		// Create a new context. Note: We can't use 'push' since we don't want to destroy the old context.
		size_t *newStack = oldStack - 13;

		// Return address for the new function. Needed for stack traces, but we don't expect we will
		// ever return there.
		newStack[12] = size_t(&doSwitchReturnLoc);

		newStack[11] = size_t(fn);
		newStack[10] = size_t(rbp);
		newStack[9] = size_t(param);
		newStack[8] = size_t(rbx);
		newStack[7] = size_t(r12);
		newStack[6] = size_t(r13);
		newStack[5] = size_t(r14);
		newStack[4] = size_t(r15);
		newStack[3] = 0;
		newStack[2] = size_t(high);
		newStack[1] = size_t(dummy);
		newStack[0] = size_t(low);

		// Set the 'desc'!
		atomicWrite(stack.desc, (StackDesc *)(newStack));

		return old;
	}

	void UThreadData::restoreContext(StackDesc *desc) {
		atomicWrite(stack.desc, (StackDesc *)desc);
	}

	// Called from UThreadX64.S
	extern "C" void doEndDetour2(void **result) {
		UThreadState::current()->endDetour(result);
	}

	extern "C" void doEndDetour();
	extern "C" void doEndDetourMember();

	static const void *endDetourFn(bool member) {
		return member
			? address(doEndDetourMember)
			: address(doEndDetour);
	}

#elif defined(GCC) && defined(ARM64)

	static void *&stackPtr(Stack &stack) {
		return stack.desc->low;
	}

	void UThreadData::push(void *v) {
		void **sp = (void **)stackPtr(stack);
		*--sp = v;
		stackPtr(stack) = sp;
	}

	void UThreadData::push(uintptr_t v) {
		void **sp = (void **)stackPtr(stack);
		*--sp = (void *)v;
		stackPtr(stack) = sp;
	}

	void UThreadData::push(intptr_t v) {
		void **sp = (void **)stackPtr(stack);
		*--sp = (void *)v;
		stackPtr(stack) = sp;
	}

	void UThreadData::pushContext(const void *fn) {
		pushContext(fn, null);
	}

	// Function for launching new threads. Not callable from C, so a regular variable.
	extern "C" char launchThreadStub;

	const int arm64FrameWords = 24; // Size of the arm64 context in 8-byte words.

	void UThreadData::pushContext(const void *fn, void *param) {
		void **sp = (void **)stackPtr(stack);

		// We need a bit of padding for exceptions to work. They read a few words here (maybe due to
		// incorrect unwind info?) Remember: Stack must be 16-byte aligned.
		// sp -= 2*sizeof(void *);

		sp -= arm64FrameWords;
		stackPtr(stack) = sp;

		// Zero everything.
		memset(sp, 0, sizeof(void *)*arm64FrameWords);

		// Store the state we need.
		sp[1] = &launchThreadStub; // Return to.
		sp[23] = param;
		sp[20] = stack.desc->low;
		sp[21] = stack.desc->dummy;
		sp[22] = stack.desc->high;

		// Parameters to 'launchThread':
		sp[0] = null; // base pointer of prev frame.
		sp[2] = (void *)fn; // function to call
		sp[3] = param; // parameter
		sp[4] = null; // apparent caller (none).

		// Now we can use the new Desc on the stack.
		atomicWrite(stack.desc, (StackDesc *)(sp + 20));
	}

	// Logical location where thread switches are made. Used to get good backtraces.
	// Note: This is a label in asm, so the interesting thing about this is its address.
	extern "C" char doSwitchReturnLoc;

	StackDesc *UThreadData::pushSubContext(const void *fn, void *param) {
		void **sp = (void **)stackPtr(stack);
		void *oldSp = sp;
		sp -= arm64FrameWords;
		// Note: We don't save the sp in the old desc, since we don't want to trash it!

		// Zero the stack frame.
		memset(sp, 0, sizeof(void *)*arm64FrameWords);

		// Store the state we need. Similar to pushContext.
		sp[1] = &launchThreadStub; // Return to.
		sp[20] = sp; // low of new desc.
		sp[21] = stack.desc->dummy;
		sp[22] = stack.desc->high;

		// Parameters to 'launchThread':
		sp[0] = oldSp; // stored frame pointer.
		sp[2] = (void *)fn; // function to call.
		sp[3] = param; // parameter
		sp[4] = &doSwitchReturnLoc; // apparent return address (for stack traces to work properly)

		StackDesc *old = stack.desc;

		// Set the new desc.
		atomicWrite(stack.desc, (StackDesc *)(sp + 20));

		return old;
	}

	void UThreadData::restoreContext(StackDesc *desc) {
		atomicWrite(stack.desc, (StackDesc *)desc);
	}

	// Called from ASM.
	extern "C" void doEndDetour2(void **result) {
		UThreadState::current()->endDetour(result);
	}

	extern "C" void doEndDetour();
	extern "C" void doEndDetourMember();

	static const void *endDetourFn(bool member) {
		return member
			? address(doEndDetourMember)
			: address(doEndDetour);
	}

#endif

}
