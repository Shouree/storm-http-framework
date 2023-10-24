#pragma once
#include "Object.h"
#include "Handle.h"
#include "OS/Future.h"
#include "OS/Sync.h"
#include "Core/GcTypeStore.h"

namespace storm {
	STORM_PKG(core);

	class Exception;

	/**
	 * Future-object exposed to the runtime and built into function calls. By calling
	 * 'asyncThreadCall' on a function, you will get one of these objects. It also simplifies the
	 * interface of the Future in code since it supports copying of the future (references the same
	 * result) and also correctly handles cases where the result is not waited for.
	 *
	 * The future takes care of any copying of the objects.
	 */
	class FutureBase : public Object {
		STORM_CLASS;
	public:
		// Create.
		FutureBase(const Handle &type);

		// Copy ctor.
		FutureBase(const FutureBase &o);

		// Destructor to keep the reference count of Data.
		~FutureBase();

		// Deep copy.
		virtual void STORM_FN deepCopy(CloneEnv *env);

		// Post a result.
		void CODECALL postRaw(const void *value);

		// Post an error (assuming we're inside a catch-block).
		void error();

		// Post an error.
		void STORM_FN error(Exception *exception);

		// Wait for the result. 'to' is empty memory where the value will be copied into.
		void CODECALL resultRaw(void *to);

		// Get the result as an exception if it was an exception, otherwise just return whenever we have a result.
		void STORM_FN errorResult();

		// Detach the future, ie. tell it we don't care about the result.
		void STORM_FN detach();

		// Get the underlying future object. Note: when you call this function,
		// you are required to call either postRaw or error on the future object, otherwise
		// we will leak resources!
		os::FutureBase *rawFuture();

		// Get the place where the result is to be stored.
		void *rawResult() { return data->result->v; }

		// Tell this future instance that it does not need to clone the result before returning it.
		// This is set when we have a future that we know will be posted on the same Thread as the
		// current thread. We will clear this flag as soon as "deepCopy" is called (which indicates
		// that we crossed a Thread boundary).
		void CODECALL setNoClone();

	private:
		// Custom extension of the 'FutureSema' object, so that we may get a notification when a
		// result has been posted.
		class FutureSema : public os::FutureSema<os::Sema> {
		public:
			// Ctor.
			FutureSema();

		protected:
			// Custom notify so that we can keep the Data struct alive long enough.
			virtual void notify();
		};

		// Data shared between futures. Since we allow copies, we need to share one FutureBase
		// object. This struct is allocated on the non-moving GC heap since it contains potentially
		// sensitive constructs (like OS semaphores and locks).
		//
		// We use reference counting here to get slightly more prompt destruction of the data
		// object. This is since the FutureBase object is allocated in a generational pool, which
		// is collected more frequently, while the Data object is in a non-moving pool that is
		// rarely collected.  This way, we can at least free the OS resources earlier while making
		// sure that we don't accidentally leak memory in certain situations (the finalizer on Data
		// is a backup).
		struct Data {
			Data(const Handle &handle, GcArray<byte> *result);

			~Data();

			// The Future object we are using.
			FutureSema future;

			// Handle.
			const Handle *handle;

			// Result. We use 'filled' to determine if anything is stored here.
			GcArray<byte> *result;

			// References.
			nat refs;

			// Release on receiving a result?
			nat releaseOnResult;

			// Add/release references.
			void addRef();
			void release();

			// Called when the semaphore is notified.
			static void resultPosted(FutureSema *sema);

			// Finalizer for this type.
			static void finalize(void *object, os::Thread *thread);

			// GC description for this type.
			static const GcTypeStore<3> gcType;
		};

		// Pointer to the data.
		UNKNOWN(PTR_GC) Data *data;

		// Can we skip cloning the result?
		Bool noClone;
	};

	// Declare the template.
	STORM_TEMPLATE(Future, createFuture);

	/**
	 * Specialized class, compatible with the template instantiations generated
	 * by the Storm runtime.
	 */
	template <class T>
	class Future : public FutureBase {
		STORM_SPECIAL;
	public:
		// Type lookup.
		static Type *stormType(Engine &e) {
			return runtime::cppTemplate(e, FutureId, 1, StormInfo<T>::id());
		}

		// Create.
		Future() : FutureBase(StormInfo<T>::handle(engine())) {
			runtime::setVTable(this);
		}

		// Copy.
		Future(const Future<T> *o) : FutureBase(o) {
			runtime::setVTable(this);
		}

		// Post result.
		void post(T t) {
			cloned(t, new (this) CloneEnv());
			postRaw(&t);
		}

		// Get result.
		T result() {
			byte data[sizeof(T)];
			resultRaw(data);
			T copy = *(T *)data;
			((T *)data)->~T();
			return copy;
		}
	};

	/**
	 * Special case for 'void'
	 */
	template <>
	class Future<void> : public FutureBase {
		STORM_SPECIAL;
	public:
		// Type lookup.
		static Type *stormType(Engine &e) {
			return runtime::cppTemplate(e, FutureId, 1, StormInfo<void>::id());
		}

		// Create.
		Future() : FutureBase(StormInfo<void>::handle(engine())) {
			runtime::setVTable(this);
		}

		// Copy.
		Future(const Future<void> &o) : FutureBase(o) {
			runtime::setVTable(this);
		}

		// Post.
		void post() {
			postRaw(null);
		}

		// Get result.
		void result() {
			resultRaw(null);
		}
	};

}
