#pragma once
#include "Core/StrBuf.h"
#include "NamedThread.h"

namespace storm {
	STORM_PKG(core.lang);

	/**
	 * Specifies on which thread a specific function is to be run.
	 */
	class RunOn {
		STORM_VALUE;
	public:
		enum State {
			// Run anywhere. This is used when no threading constraint has been declared, and is the
			// default value of this class.
			any,

			// The thread to run on is decided at runtime. For example, when a class has been
			// declared to run on a thread provided runtime.
			runtime,

			// The thread is known compile-time. Find the thread in the 'thread' member.
			named,
		};

		// State.
		State state;

		// Thread. Only valid if 'state == named'.
		MAYBE(NamedThread *) thread;

		// Create.
		STORM_CTOR RunOn();
		STORM_CTOR RunOn(State s);
		STORM_CTOR RunOn(NamedThread *thread);

		// Assuming we're running on a thread represented by 'this', may we run a function declared
		// to run on 'other' without sending messages?
		Bool STORM_FN canRun(RunOn other) const;

		// Output.
		void STORM_FN toS(StrBuf *to) const;
	};

	// Output.
	wostream &operator <<(wostream &to, const RunOn &v);

}
