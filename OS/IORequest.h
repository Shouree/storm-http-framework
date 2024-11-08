#pragma once
#include "Sync.h"
#include "Handle.h"
#include "UThread.h"

namespace os {

	class Thread;
	class IORequest;

	/**
	 * Sleep structure for IORequests that have a timeout.
	 */
	class IOTimeoutSleep : public UThreadState::SleepData {
	public:
		IOTimeoutSleep() : SleepData(0), request(null) {}

		IORequest *request;

		virtual void signal();
	};


	/**
	 * IO request. These are posted to an IOHandle when the requests are completed.
	 */
#if defined(WINDOWS)

	class IORequest : public OVERLAPPED {
	public:
		// Note the thread that the request is associated with.
		IORequest(Handle handle, const Thread &thread, nat timeout = 0);
		~IORequest();

		// Sema used for notifying when the request is complete.
		Sema wake;

		// Handle we are working with. So that we can cancel IO.
		Handle handle;

		// Number of bytes read.
		nat bytes;

		// Error code (if any).
		int error;

		// Called on completion.
		void complete(nat bytes);

		// Called on failure.
		void failed(nat bytes, int error);

	private:
		// Owning thread.
		const Thread &thread;

		// Sleep structure.
		IOTimeoutSleep sleep;
	};

#elif defined(POSIX)

	class IORequest {
	public:
		// Read/write?
		enum Type {
			read, write
		};

		// Note the thread that the request is associated with.
		IORequest(Handle handle, Type type, const Thread &thread, nat timeout = 0);
		~IORequest();

		// Event used for notifying when the file descriptor is ready for the desired operation, or
		// when it is closed.
		Event wake;

		// Request type (read/write).
		Type type;

		// Closed?
		bool closed;

		// Timeout?
		bool timeout;

	private:
		// Handle used.
		Handle handle;

		// Owning thread.
		const Thread &thread;

		// Sleep structure.
		IOTimeoutSleep sleep;
	};

#else
#error "Implement IORequest for your platform!"
#endif

}
