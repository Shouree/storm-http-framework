#include "stdafx.h"
#include "Thread.h"
#include "UThread.h"
#include "ThreadGroup.h"
#include "Shared.h"

#ifdef WINDOWS
#include <process.h>
// For COM - CoInitializeEx
#include <Objbase.h>
#endif

namespace os {

	// Data to a started thread.
	class ThreadStart {
	public:
		// Semaphore to indicate successful start.
		Semaphore sema;

		// Pointer to the ThreadData object for the current thread.
		// Valid when 'sema' has been decreased.
		ThreadData *data;

		// Function to execute.
		const util::Fn<void, void> &startFn;

		// ThreadWait behavior. May be null.
		ThreadWait *wait;

		// Thread group.
		ThreadGroupData *group;

		// Init.
		ThreadStart(const util::Fn<void, void> &fn, ThreadWait *wait, ThreadGroupData *group) :
			sema(0), startFn(fn), wait(wait), group(group) {}

		// Remove one reference from 'data', if present. The started thread
		// should set one reference in 'data', so that it may not exit
		// before the 'start' function has returned and reclaimed at least one
		// reference.
		~ThreadStart() {
			if (data)
				data->release();
		}
	};

	// This one is system-specific
	static void startThread(ThreadStart &start);

	// Capture thread- and OS-specific data for each thread.
	static void *captureOSExtraData();

	/**
	 * Thread data.
	 */

	ThreadData::ThreadData(void *stack, void *extra) :
		references(0), uState(this, stack), osExtra(extra) {}

	ThreadData::~ThreadData() {}

	void ThreadData::reportZero() {
		wakeCond.signal();
	}

	/**
	 * Thread
	 */

	Thread::Thread(ThreadData *data) : data(data) {
		if (data)
			data->addRef();
	}

	Thread::Thread(const Thread &o) : data(o.data) {
		if (data)
			data->addRef();
	}

	Thread::~Thread() {
		if (data)
			data->release();
	}

	Thread &Thread::operator =(const Thread &o) {
		if (data)
			data->release();
		data = o.data;
		if (data)
			data->addRef();
		return *this;
	}

	wostream &operator <<(wostream &to, const Thread &o) {
		return to << L"thread @" << o.data;
	}

	void Thread::attach(Handle h) const {
		data->attach(h);
	}

	void Thread::detach(Handle h) const {
		data->detach(h);
	}

	static void *mainStackBase = null;
	static void *mainStackOSData = null;

	Thread Thread::current() {
		ThreadData *t = currentThreadData();
		if (t)
			return Thread(t);

		assert(mainStackBase, L"Call 'Thread::setStackBase' before using 'Thread::current'");

		// The first thread, create its data...
		static ThreadData firstData(mainStackBase, mainStackOSData);
		// Keep the first thread from firing 'signal' all the time.
		static Thread first(&firstData);

		// Store it in the thread local variable. Otherwise "current" will not work when called from
		// other shared objects, since 'mainStackBase' is static to this file.
		currentThreadData(&firstData);

		return first;
	}

	void Thread::setStackBase(void *base) {
		mainStackBase = base;
		mainStackOSData = captureOSExtraData();
	}

	Thread Thread::invalid = Thread(null);

	const InlineSet<Stack> &Thread::stacks() const {
		return data->uState.stacks;
	}

	Thread Thread::spawn(ThreadGroup &group) {
		return spawn(util::Fn<void>(), group);
	}

	Thread Thread::spawn(const util::Fn<void, void> &fn, ThreadGroup &group) {
		ThreadStart start(fn, null, group.data);
		startThread(start);
		start.sema.down();

		Thread t(start.data);
		// Consume the additional reference, making sure the thread does not terminate before 'spawn' returns.
		start.data->release();
		return t;
	}

	Thread Thread::spawn(ThreadWait *wait, ThreadGroup &group) {
		util::Fn<void, void> f;
		ThreadStart start(f, wait, group.data);
		startThread(start);
		start.sema.down();

		Thread t(start.data);
		// Consume the additional reference, making sure the thread does not terminate before 'spawn' returns.
		start.data->release();
		return t;
	}

	void ThreadData::threadMain(ThreadStart &start, void *stackBase) {
		ThreadData d(stackBase, captureOSExtraData());
		d.addRef(); // Add a reference so that 'd' do not terminate prematurely.

		Thread::initThread();
		threadCreated();

		// Read data from 'start'.
		ThreadGroupData *group = start.group;
		util::Fn<void, void> fn = start.startFn;
		ThreadWait *wait = start.wait;
		d.wait = wait;

		// One reference is consumed when 'threadWait' is terminated.
		d.addRef();
		// One is used to prevent signaling the semaphore before we have finished doing our main job.
		d.addRef();

		// Remember our identity.
		currentThreadData(&d);

		// Initialize our group.
		group->addRef();
		group->threadStarted(&d);

		// Initialize any 'wait' struct before anyone is able to call 'signal' on it.
		if (wait)
			wait->init();

		// Report back.
		start.data = &d;
		start.sema.up();
		// From here on, do not touch 'start'.

		// Any other initialization required before we start executing code?
		if (wait)
			wait->setup();

		// Run the function we were told to execute.
		fn();

		// Specific wait behavior?
		if (wait) {
			do {
				// Run any spawned UThreads, interleave with anything we need to do.
				do {
					if (d.wait)
						wait->work();
				} while (UThread::leave());

			} while (d.wait && d.waitForWork());

			// Clean up the 'wait' structure.
			d.wait = null; // No more notifications, but we can not delete it yet!
		}

		// Go back to zero references, so that we may terminate!
		d.release();

		while (true) {
			// Either we have more references, or more UThreads to run.
			// Either way, it does not hurt to try to run the UThreads.
			while (UThread::leave())
				;

			// If the refcount is zero, we can safely say that no one else
			// can increase it after this point (assuming no UThreads).
			// At that point no one can add more UThreads either, so in
			// this case we can not have any race-conditions.

			if (atomicRead(d.references) == 0) {
				if (!UThread::any()) {
					// Attempt to shutdown!
					if (group->threadUnreachable(&d))
						break;

					// If we get here, the thread group handed out a new reference while we were
					// looking, and we need to continue working for a while.
				}
			}

			// Wait for the condition to fire. This is done whenever the
			// refcount reaches zero, or when a new UThread has been added.
			d.waitForWork();
		}

		// Now, no one has any knowledge of our existence, we can safely delete the 'wait' now.
		delete wait;

		// Report we terminated.
		group->threadTerminated();
		group->release();

		// Failsafe for the currThreadData.
		currentThreadData(null);

		d.ioComplete.close();
		threadTerminated();
		Thread::cleanThread();
	}

	void ThreadData::reportWake() {
		// We need to signal both in case we're in the process of exiting from the 'wait' behaviour.
		wakeCond.signal();
		if (wait)
			wait->signal();
	}

	bool ThreadData::waitForWork() {
		bool result = false;
		checkIo();

		nat sleepFor = 0;
		if (uState.nextWake(sleepFor)) {
			if (sleepFor > 0) {
				if (wait) {
					if (!wait->wait(ioComplete, sleepFor))
						wait = null;
					else
						result = true;
				} else {
					wakeCond.wait(ioComplete, sleepFor);
				}
			} else if (wait) {
				// Just assume 'wait' shall remain, since we did not ask it about its desires.
				result = true;
			}
			uState.wakeThreads();
		} else {
			if (wait) {
				if (!wait->wait(ioComplete))
					wait = null;
				else
					result = true;
			} else {
				wakeCond.wait(ioComplete);
			}
		}

		checkIo();
		return result;
	}

	void ThreadData::checkIo() const {
		ioComplete.notifyAll(this);
	}

#ifdef WINDOWS
	// Windows-specific implementation.

#ifdef X86

	// If we are on X86, there is an option to enable another layer on top of SAFESEH: namely, that
	// the system examines the last element of the SEH linked list and verifies that it points to a
	// particular function (FinalExceptionHandler in ntdll, there are a number of offsets there, and
	// we probably need to point to the right one).

	struct SehRecord {
		SehRecord *prev;
		void *handler;
	};

	static void *captureOSExtraData() {
		// Capture the top of the SEH chain.
		SehRecord *first = null;
		__asm {
			mov eax, fs:[0];
			mov first, eax;
		}

		// Traverse it until we reach 0xFFFFFFFF, and remember the last handler.
		void *lastHandler = null;
		while (size_t(first) != size_t(-1)) {
			lastHandler = first->handler;
			first = first->prev;
		}

		return lastHandler;
	}

#else

	static void *captureOSExtraData() {
		return null;
	}

#endif

	static void winThreadMain(void *param) {
		ThreadStart *s = (ThreadStart *)param;
		ThreadData::threadMain(*s, &param);
	}

	static void startThread(ThreadStart &start) {
		_beginthread(&winThreadMain, 0, &start);
	}

	void Thread::initThread() {
		CoInitializeEx(NULL, COINIT_APARTMENTTHREADED | COINIT_SPEED_OVER_MEMORY);
	}

	void Thread::cleanThread() {
		CoUninitialize();
	}

#else

	static void *posixThreadMain(void *param) {
		ThreadStart *s = (ThreadStart *)param;
		ThreadData::threadMain(*s, &param);
		return null;
	}

	static void startThread(ThreadStart &start) {
		pthread_t thread;
		pthread_create(&thread, null, &posixThreadMain, &start);
		pthread_detach(thread);
	}

	static void *captureOSExtraData() {
		return null;
	}

	void Thread::initThread() {}

	void Thread::cleanThread() {}

#endif

	ThreadWait::~ThreadWait() {}

	void ThreadWait::init() {}

	void ThreadWait::setup() {}

	void ThreadWait::work() {}

}
