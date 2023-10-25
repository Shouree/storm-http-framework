#pragma once
#include "Core/StackTrace.h"
#include "Core/Thread.h"
#include "Core/GcArray.h"

namespace storm {
	STORM_PKG(core);

	/**
	 * A set of stack traces. One for each UThread for a single OS thread.
	 *
	 * Automatically formats the output when printed.
	 */
	class ThreadStackTraces : public Object {
		STORM_CLASS;
	public:
		// Create the set of stack traces that is initially empty.
		ThreadStackTraces(Nat count);

		// The thread that these stack traces were captured for. It is not always possible to
		// capture this information.
		MAYBE(Thread *) thread;

		// The name of the thread that these stack traces were captured for, if it was found.
		MAYBE(Str *) threadName;

		// The thread ID of this thread.
		Word threadId;

		// Get the number of stack traces here.
		Nat STORM_FN count() const;

		// Get a single stack trace.
		StackTrace STORM_FN operator [](Nat elem) const;

		// Add a stack trace. Returns the ID of it.
		Nat push(StackTrace trace);

		// To string.
		void STORM_FN toS(StrBuf *to) const;

	private:
		// Stack trace data.
		GcArray<StackTrace> *data;
	};

	/**
	 * A set of stack traces for a set of threads. Essentially an array of ThreadStackTraces.
	 *
	 * Automatically formats the output when printed.
	 */
	class MultiThreadStackTraces : public Object {
		STORM_CLASS;
	public:
		// Create the set. Not exposed to Storm since creating a set will create it in an invalid
		// state until it is initialized.
		MultiThreadStackTraces(Nat count);

		// Get the number of threads.
		Nat STORM_FN count() const;

		// Get a single element.
		ThreadStackTraces *STORM_FN operator [](Nat elem) const;

		// Set a specific trace ID.
		void set(Nat id, ThreadStackTraces *elem);

		// To string.
		void STORM_FN toS(StrBuf *to) const;

	private:
		// Data.
		GcArray<ThreadStackTraces *> *data;
	};

	// Capture all stack traces for the current OS thread. Note: this is a fairly expensive operation.
	ThreadStackTraces *STORM_FN collectThreadStackTraces(EnginePtr e);

	// Capture all stack traces for all the threads in the array. Note: this is a fairly expensive operation.
	MultiThreadStackTraces *STORM_FN collectThreadStackTraces(EnginePtr e, Array<Thread *> *threads);

	// Capture all stack traces for all threads in the system. Note: this is a very expensive operation.
	MultiThreadStackTraces *STORM_FN collectAllStackTraces(EnginePtr e);

}
