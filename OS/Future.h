#pragma once
#include "Utils/TypeInfo.h"
#include "Utils/Semaphore.h"
#include "PtrThrowable.h"

// In Visual Studio 2008, we do not have exception_ptr, so we implement our own
// solution in that case!
#if defined(VISUAL_STUDIO) && VISUAL_STUDIO <= 2008
#define CUSTOM_EXCEPTION_PTR
#else
#include <stdexcept>
#endif

namespace os {
	class Sema;

#ifdef CUSTOM_EXCEPTION_PTR
	struct CppExceptionType;
#endif

	/**
	 * Future that stores the result in some unrelated memory. This class is designed to be possible
	 * to use with minimal type information and to be easy to use from machine code or general
	 * non-templated C++ code. Therefore it is not very type-safe at all. For a more type-safe and
	 * user-friendly version, see the Future<> class below. The default implementation of FutureBase
	 * is this one (specialized with <void>). This is abstract, as it requires the use of a specific
	 * semaphore type.
	 */
	class FutureBase : NoCopy {
	public:
		// Create the future, indicating a location for the result.
		FutureBase();

		// Detect unhandled exceptions and print them.
		~FutureBase();

		// Wait for the result. This function will either return normally, indicating the result is
		// written to 'target', or throw an exception posted.
		void result();

		// Same as above, but provides an opportunity to modify any pointer-based exception before
		// it is thrown.
		typedef PtrThrowable *(*InterceptFn)(PtrThrowable *original, void *env);
		void result(InterceptFn fn, void *env);

		// Detach this exception, ie. don't complain about uncaught errors.
		void detach();

		// Tell the waiting thread we have posted a result.
		void posted();

		// Post an error. This function must be called from within a throw-catch block. Otherwise
		// the runtime crashes. The 'ptrStorage' pointer shall refer to memory where it is safe to
		// store pointers to garbage collected objects.
		void error();

		// Has any result been posted?
		bool dataPosted();

		// Any result (either data or error) posted?
		bool anyPosted();

		// Pointer-based exceptions are stored here so that they can be garbage collected properly.
		PtrThrowable *ptrException;

	protected:
		// Manage the notification between threads (whichever implementation is used).
		virtual void notify() = 0;
		virtual void wait() = 0;

	private:

		// Throw the error captured earlier.
		void throwError();

		// Throw the error captured earlier, knowing it is a pointer.
		void throwPtrError(InterceptFn fn, void *env);

		// Save the current exception.
		void saveError();

		// Save the current exception, knowing it is a pointer.
		void savePtrError(const PtrThrowable *exception);

		// Perform any cleanup needed.
		void cleanError();

#ifdef CUSTOM_EXCEPTION_PTR
		class ExceptionPtr {
		public:
			ExceptionPtr();
			~ExceptionPtr();

			void *data;
			const CppExceptionType *type;

			// Only used on 64-bit systems.
			HINSTANCE instance;

			// Rethrow this exception.
			void rethrow();
			void rethrowPtr(PtrThrowable *&ptr);

			// Clear the data here.
			void clear();

			// Copy from an exception.
			void set(void *from, const CppExceptionType *type, HINSTANCE instance);

			// Copy a pointer from an exception.
			void setPtr(void *from, const CppExceptionType *type, HINSTANCE instance, PtrThrowable *&store);
		};

		// The exception.
		ExceptionPtr exceptionData;

		// Filter expression function.
		int filter(EXCEPTION_POINTERS *ptrs, bool pointer);

#elif defined(POSIX)

		// Exception data. Either an std::exception ptr (if it was a C++ exception), or a
		// std::type_info * if it was a pointer exception.
		size_t exceptionData[sizeof(std::exception_ptr) / sizeof(size_t)];

#else
		int filter(EXCEPTION_POINTERS *ptrs, InterceptFn fn, void *env);

		// Regular implementation, we use filter functions to modify the data.
		std::exception_ptr exceptionData;

#endif

		// Any error?
		bool hasError() const {
			nat r = atomicRead(resultPosted);
			return r == resultError || r == resultErrorPtr;
		}

		// What kind of result was posted?
		enum {
			// No result.
			resultEmpty,

			// A regular value.
			resultValue,

			// A generic C++ exception.
			resultError,

			// A pointer to PtrThrowable.
			resultErrorPtr,
		};

		// Anything posted? Any of 'result*'
		nat resultPosted;

		// Did we read the result?
		enum {
			// Not read.
			readNone,

			// Read at least once.
			readOnce,

			// Detached. i.e. will not be read.
			readDetached,
		};

		// Anything read?
		nat resultRead;

		// Warn about errors from detached threads.
		void warnDetachedError();
	};

	/**
	 * Specify which semaphore to use. T is either Semaphore or Sema, or something compatible.
	 */
	template <class T>
	class FutureSema : public FutureBase {
	public:
		// Create, just like above.
		FutureSema() : FutureBase(), sema(0) {}

	protected:
		// The semaphore to use.
		T sema;

		virtual void wait() {
			sema.down();
			sema.up(); // allows us to wait more than once!
		}

		virtual void notify() {
			sema.up();
		}
	};

	/**
	 * A real C++ friendly implementation of the future. Defaults to use the
	 * semaphore with support for background threads. This class does not
	 * inherit from the FutureBase<> class to provide a cleaner interface
	 * to the user. The underlying FutureBase<> object may still be queried
	 * if neccessary.
	 * This class assumes that you will call 'result' at least once.
	 */
	template <class T, class Sema = os::Sema>
	class Future : NoCopy {
	public:
		// Create.
		Future() {}

		// Destroy.
		~Future() {
			if (future.dataPosted()) {
				void *v = value;
				((T *)v)->~T();
			}
		}

		// Get the result or throw the error, when the thread is ready.
		T result() {
			future.result();
			void *v = value;
			return *(T *)v;
		}

		// Post the result.
		void post(const T &result) {
			new (value) T(result);
			future.posted();
		}

		// Post an error. Only call from a catch-block.
		void error() {
			future.error();
		}

		// Get the underlying Future object.
		FutureBase &impl() {
			return future;
		}

		// Get our data.
		void *data() {
			return value;
		}

	private:
		// Underlying object.
		FutureSema<Sema> future;

		// Result storage. Note that we do not want to initialize it before
		// 'future' tries to return something into this.
		byte value[sizeof(T)];
	};

	/**
	 * Special case for void values (exceptions and syncronization may still be interesting!).
	 */
	template <class Sema>
	class Future<void, Sema> : NoCopy {
	public:
		// Create.
		Future() {}

		// Destroy.
		~Future() {}

		// Get the result or throw the error, when the thread is ready.
		void result() {
			future.result();
		}

		// Post the result.
		void post() {
			future.posted();
		}

		// Post an error. Only call from a catch-block.
		void error() {
			future.error();
		}

		// Get the underlying Future object.
		FutureBase &impl() {
			return future;
		}

		// Get our data.
		void *data() {
			return null;
		}

	private:
		// Underlying object.
		FutureSema<Sema> future;
	};

}
