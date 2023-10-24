#pragma once
#include "Object.h"

namespace storm {
	STORM_PKG(core);

	/**
	 * A thread known by Storm.
	 *
	 * NOTE: due to the startup process, this thread may *not* contain any pointers to other Storm
	 * objects, as they are not reported to the GC during startup.
	 *
	 * NOTE: These may be copied, so never assume that if a != b, then they represent different
	 * threads. However, the reverse holds. Ie. if a == b then they represent the same thread.
	 */
	class Thread : public Object {
		STORM_CLASS;
	public:
		// Create a thread.
		STORM_CTOR Thread();

		// Create from a previously started os::Thread object. This thread must have called the
		// 'register' function previously.
		Thread(const os::Thread &thread);

		// Lazily create the underlying thread when needed.
		Thread(DeclThread::CreateFn fn);

		// Make sure that the previous thread has been created during a copy.
		Thread(const Thread &o);

		// Destroy.
		~Thread();

		// Deep copy.
		virtual void STORM_FN deepCopy(CloneEnv *env);

		// Check for equality.
		Bool STORM_FN operator ==(const Thread &o) const;

		// Compute a hash.
		Nat STORM_FN hash() const;

		// Get the thread handle.
		const os::Thread &thread();

		// Check if this is the same thread as the one passed as a parameter. This will not cause
		// any threads to be created.
		bool sameAs(const os::Thread &other) const;

#ifdef STORM_COMPILER
		/**
		 * Allow stand-alone allocation of the first Thread.
		 */
		struct First {
			Engine &e;
			First(Engine &e) : e(e) {}
		};

		static void *operator new(size_t size, First t);
		static void operator delete(void *mem, First t);
#endif

	private:
		// Thread handle.
		UNKNOWN(PTR_NOGC) os::Thread osThread;

		// Create using.
		UNKNOWN(PTR_NOGC) DeclThread::CreateFn create;
	};


	/**
	 * The main thread of the compiler.
	 *
	 * In order to avoid many potential threading issues in the compiler, the compiler itself is
	 * single threaded and executed by this thread. This is important to consider when writing
	 * languages and other code that deal with compilation. Even though the languages in Storm will
	 * ensure that your progam is properly synchronized with regards to data races, it is beneficial
	 * to place logic that relies a lot on functionality provided by the compiler itself (such as
	 * the name tree) on the Compiler thread as well in order to avoid an excessive amount of
	 * expensive thread switches.
	 */
	STORM_THREAD(Compiler);

}
