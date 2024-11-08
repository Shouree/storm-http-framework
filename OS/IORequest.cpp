#include "stdafx.h"
#include "IORequest.h"
#include "Thread.h"

namespace os {

#ifdef WINDOWS

	IORequest::IORequest(Handle handle, const Thread &thread, nat timeout)
		: wake(0), handle(handle), bytes(0), error(0), thread(thread) {

		Internal = 0;
		InternalHigh = 0;
		Offset = 0;
		OffsetHigh = 0;
		Pointer = NULL;
		hEvent = NULL;
		thread.threadData()->ioComplete.attach();

		if (timeout) {
			sleep.until = UThreadState::sleepTarget(timeout);
			sleep.request = this;
			thread.threadData()->uState.addSleep(&sleep);
		}
	}

	IORequest::~IORequest() {
		if (sleep.request)
			thread.threadData()->uState.cancelSleep(&sleep);
		thread.threadData()->ioComplete.detach();
	}

	void IORequest::complete(nat bytes) {
		this->bytes = bytes;
		this->error = 0;
		wake.up();
	}

	void IORequest::failed(nat bytes, int error) {
		this->bytes = bytes;
		this->error = error;
		wake.up();
	}

	void IOTimeoutSleep::signal() {
		if (request) {
			// This causes this particular IO request to be cancelled as normal. That means we don't
			// have to do anything special to handle it, even if it was partially completed.
			CancelIoEx(request->handle.v(), request);
		}
		request = null;
	}

#endif

#ifdef POSIX

	IORequest::IORequest(Handle handle, Type type, const Thread &thread, nat timeout)
		: type(type), closed(false), timeout(false), handle(handle), thread(thread) {

		thread.threadData()->ioComplete.attach(handle, this);
		if (timeout) {
			sleep.until = UThreadState::sleepTarget(timeout);
			sleep.request = this;
			thread.threadData()->uState.addSleep(&sleep);
		}
	}

	IORequest::~IORequest() {
		if (sleep.request)
			thread.threadData()->uState.cancelSleep(&sleep);
		thread.threadData()->ioComplete.detach(handle, this);
	}

	void IOTimeoutSleep::signal() {
		if (request) {
			// We can just cancel it immediately. On UNIX, we always rely on OS-level buffers, so
			// nothing is transferred outside of the calls to read/write.
			request->timeout = true;
			request->wake.set();
		}
		request = null;
	}

#endif

}
