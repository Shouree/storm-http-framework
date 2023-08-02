#include "stdafx.h"
#include "StdIoThread.h"

namespace storm {

	StdIo::StdIo() : first(null), last(null) {
		platformInit();
	}

	StdIo::~StdIo() {
		if (hasThread()) {
			StdRequest quit(stdIn, null, 0);
			postThread(&quit);

			// Wait for the thread to terminate.
			waitThread();
		}
		platformDestroy();
	}

	void StdIo::doNop(StdRequest *r) {
		r->count = 0;
		r->wait.up();
	}

	void StdIo::post(StdRequest *r) {
		// No accidental 'terminate' messages.
		if (!r->buffer) {
			doNop(r);
			return;
		}

		switch (r->stream) {
		case stdIn: {
			// See if we can read now, otherwise post to the thread.
			bool ok = false;
			if (tryLockInput()) {
				ok = tryRead(r);
				unlockInput();
			}

			if (!ok)
				postThread(r);
			break;
		}
		case stdOut:
		case stdError:
			// Output to the streams directly.
			// TODO: We should queue these requests if we realize they will take time.
			doWrite(r);
			break;
		default:
			// Invalid stream...
			doNop(r);
			break;
		}
	}

	void StdIo::postThread(StdRequest *r) {
		{
			util::Lock::L z(lock);
			if (last == null) {
				// One and only node!
				last = first = r;
			} else {
				last->next = r;
				last = r;
			}

			// Is the thread actually started?
			if (!hasThread())
				startThread();
		}

		cond.signal();
	}

	void StdIo::main() {
		// Note: currently input is blocking output, which is bad. We should do something clever about that!

		while (true) {
			StdRequest *now = null;
			{
				util::Lock::L z(lock);
				now = first;
				if (now)
					first = now->next;
				if (last == now)
					last = null;
			}

			// Anything to do?
			if (now == null) {
				cond.wait();
				continue;
			}

			// Got a request!
			if (now->buffer == null) {
				// We shall terminate...
				return;
			}

			if (now->stream == stdIn) {
				// Special care is needed when reading...
				lockInput();
				doRead(now);
				unlockInput();
			} else {
				doWrite(now);
			}
		}
	}

#if defined(WINDOWS)

	void StdIo::platformInit() {
		thread = NULL;
		handles[stdIn] = GetStdHandle(STD_INPUT_HANDLE);
		handles[stdOut] = GetStdHandle(STD_OUTPUT_HANDLE);
		handles[stdError] = GetStdHandle(STD_ERROR_HANDLE);
		InitializeCriticalSection(&inputLock);
	}

	void StdIo::platformDestroy() {
		DeleteCriticalSection(&inputLock);
	}

	void StdIo::startThread() {
		thread = CreateThread(NULL, 1024 * 3, &ioMain, this, 0, NULL);
	}

	void StdIo::waitThread() {
		WaitForSingleObject(thread, 500);
	}

	bool StdIo::hasThread() {
		return thread != NULL;
	}

	void StdIo::lockInput() {
		EnterCriticalSection(&inputLock);
	}

	bool StdIo::tryLockInput() {
		return TryEnterCriticalSection(&inputLock) ? true : false;
	}

	void StdIo::unlockInput() {
		LeaveCriticalSection(&inputLock);
	}

	void StdIo::doRead(StdRequest *r) {
		HANDLE handle = handles[r->stream];
		DWORD result = 0;
		ReadFile(handle, r->buffer, r->count, &result, NULL);
		r->count = Nat(result);
		r->wait.up();
	}

	bool StdIo::tryRead(StdRequest *r) {
		// NOTE: This does not seem to work... At least not when inside of Emacs...
		return false;

		HANDLE handle = handles[r->stream];
		// Stdin handle becomes signaling when there is data to read.
		if (WaitForSingleObject(handle, 0) == WAIT_OBJECT_0) {
			// Safe to read!
			doRead(r);
			return true;
		} else {
			return false;
		}
	}

	void StdIo::doWrite(StdRequest *r) {
		HANDLE handle = handles[r->stream];
		DWORD result = 0;
		WriteFile(handle, r->buffer, r->count, &result, NULL);
		r->count = Nat(result);
		r->wait.up();
	}

	DWORD WINAPI ioMain(void *param) {
		StdIo *me = (StdIo *)param;
		me->main();
		return 0;
	}

#elif defined(POSIX)

	void StdIo::platformInit() {
		threadStarted = false;
		handles[stdIn] = STDIN_FILENO;
		handles[stdOut] = STDOUT_FILENO;
		handles[stdError] = STDERR_FILENO;
		pthread_mutex_init(&inputLock, null);
	}

	void StdIo::platformDestroy() {
		pthread_mutex_destroy(&inputLock);
	}

	void *ioMain(void *param);

	void StdIo::startThread() {
		// TODO? Reduce stack space for this thread?
		pthread_create(&thread, NULL, &ioMain, this);
		threadStarted = true;
	}

	void StdIo::waitThread() {
		// TODO? Add timeout?
		pthread_join(thread, NULL);
	}

	bool StdIo::hasThread() {
		return threadStarted;
	}

	void StdIo::lockInput() {
		pthread_mutex_lock(&inputLock);
	}

	bool StdIo::tryLockInput() {
		return pthread_mutex_trylock(&inputLock) == 0;
	}

	void StdIo::unlockInput() {
		pthread_mutex_unlock(&inputLock);
	}

	void StdIo::doRead(StdRequest *r) {
		int handle = handles[r->stream];
		ssize_t result = read(handle, r->buffer, r->count);
		if (result < 0)
			r->count = 0;
		else
			r->count = Nat(result);
		r->wait.up();
	}

	bool StdIo::tryRead(StdRequest *r) {
		// TODO: Temporarily switch to nonblocking IO for this fd?
		return false;
	}

	void StdIo::doWrite(StdRequest *r) {
		int handle = handles[r->stream];
		ssize_t result = write(handle, r->buffer, r->count);
		if (result < 0)
			r->count = 0;
		else
			r->count = Nat(result);
		r->wait.up();
	}

	void *ioMain(void *param) {
		StdIo *me = (StdIo *)param;
		me->main();
		return null;
	}

#endif

}
