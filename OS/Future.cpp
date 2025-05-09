#include "stdafx.h"
#include "Future.h"
#include "PtrThrowable.h"
#include "Utils/TIB.h"

namespace os {

	FutureBase::FutureBase() : ptrException(null), resultPosted(resultEmpty), resultRead(readNone) {}

	FutureBase::~FutureBase() {
		// Warn about uncaught exceptions if we didn't throw it yet.
		if (resultRead == readNone) {
			switch (atomicRead(resultPosted)) {
			case resultError:
				try {
					throwError();
				} catch (const ::Exception &e) {
					PLN(L"Unhandled exception from abandoned future:\n" << e);
				} catch (const PtrThrowable *e) {
					PLN(L"Unhandled exception from abandoned future:\n" << e->toCStr());
				} catch (...) {
					PLN(L"Unknown unhandled exception from abandoned future.");
				}
				break;
			case resultErrorPtr:
				if (ptrException) {
					PLN(L"Unhandled exception from abandoned future:\n" << ptrException->toCStr());
				} else {
					PLN(L"Unhandled exception from abandoned future.");
				}
				break;
			}

			resultRead = readOnce;
		}

		// OS specific cleanup.
		cleanError();
	}

	void FutureBase::result() {
		result(null, null);
	}

	void FutureBase::result(InterceptFn fn, void *env) {
		// Block the first thread, and allow any subsequent threads to enter.
		wait();
		atomicWrite(resultRead, readOnce);

		switch (atomicRead(resultPosted)) {
		case resultError:
			throwError();
			break;
		case resultErrorPtr:
			throwPtrError(fn, env);
			break;
		}
	}

	void FutureBase::warnDetachedError() {
		switch (atomicRead(resultPosted)) {
		case resultError:
			try {
				throwError();
			} catch (const ::Exception &e) {
				PLN(L"Unhandled exception from detached future:\n" << e);
			} catch (const PtrThrowable *e) {
				PLN(L"Unhandled exception from detached future:\n" << e->toCStr());
			} catch (...) {
				PLN(L"Unknown unhandled exception from detached future.");
			}
			break;
		case resultErrorPtr:
			if (ptrException) {
				PLN(L"Unhandled exception from detached future:\n" << ptrException->toCStr());
			} else {
				PLN(L"Unhandled exception from detached future.");
			}
			break;
		}
	}

	void FutureBase::detach() {
		nat oldRead = atomicCAS(resultRead, readNone, readDetached);
		if (oldRead == readNone) {
			// Note: Since we don't check 'resultRead' and 'resultPosted' as a unit, this might give
			// us two warnings in rare cases. As this is not very harmful, it is not really worth
			// the effort to ensure atomicity here.
			warnDetachedError();
		}
	}

	bool FutureBase::anyPosted() {
		return atomicRead(resultPosted) != resultEmpty;
	}

	bool FutureBase::dataPosted() {
		return atomicRead(resultPosted) == resultValue;
	}

	void FutureBase::posted() {
		nat p = atomicCAS(resultPosted, resultEmpty, resultValue);
		assert(p == 0, L"A future may not be used more than once! this=" + ::toHex(this));
		(void)p; // Avoid warning in release mode.
		notify();
	}

	void FutureBase::error() {
		try {
			throw;
		} catch (const PtrThrowable *ex) {
			// Save the exception pointer directly. It may be GC:d.
			nat p = atomicCAS(resultPosted, resultEmpty, resultErrorPtr);
			assert(p == resultEmpty, L"A future may not be used more than once! this=" + ::toHex(this));
			(void)p; // Avoid warning in release mode.
			savePtrError(ex);
		} catch (...) {
			// Fall back on exception_ptr or similar.
			nat p = atomicCAS(resultPosted, resultEmpty, resultError);
			assert(p == resultEmpty, L"A future may not be used more than once! this=" + ::toHex(this));
			(void)p; // Avoid warning in release mode.
			saveError();
		}
		notify();

		if (atomicRead(resultRead) == readDetached) {
			// Not really atomic, worst case is that we get the warning twice. See comment in "detach".
			warnDetachedError();
		}
	}

#ifdef CUSTOM_EXCEPTION_PTR

	/**
	 * Implementation more or less copied from Boost exception_ptr.
	 */

	const nat cppExceptionCode = 0xE06D7363;
	const nat cppExceptionMagic = 0x19930520;
#if defined(X86)
	const nat cppExceptionParams = 3;

#if _MSC_VER == 1310
	const nat exceptionInfoOffset = 0x74;
#elif (_MSC_VER == 1400 || _MSC_VER == 1500)
	const nat exceptionInfoOffset = 0x80;
#else
#error "Unknown MSC version."
#endif

#elif defined(X64)
	const nat cppExceptionParams = 4;

#if _MSC_VER==1310
	const nat exceptionInfoOffset = /* probably 0xE0 - 0xC*2, but untested */;
#elif (_MSC_VER == 1400 || _MSC_VER == 1500)
	const nat exceptionInfoOffset = 0xE0;
#else
#error "Unknown MSC version."
#endif

#endif


	struct DummyException {};
	typedef int(DummyException::*CopyCtor)(void * src);
	typedef int(DummyException::*VirtualCopyCtor)(void * src, void * dst); // virtual base
	typedef void(DummyException::*Dtor)();

	union CppCopyCtor {
		CopyCtor normal;
		VirtualCopyCtor virtualCtor;
	};

	enum CppTypeFlags {
		simpleType = 1,
		virtualBaseType = 4,
	};

	// Note: pointers below are offsets relative to the HINSTANCE on 64-bit systems.

	struct CppTypeInfo {
		unsigned flags;
		DWORD typeInfoOffset;
		int thisOffset;
		int vbaseDescr;
		int vbaseOffset;
		unsigned int size;
		DWORD copyCtorOffset;

		std::type_info *typeInfo(HINSTANCE instance) const {
			return (std::type_info *)(size_t(instance) + size_t(typeInfoOffset));
		}
		CppCopyCtor copyCtor(HINSTANCE instance) const {
			size_t ptr = size_t(instance) + size_t(copyCtorOffset);
			CppCopyCtor result;
			result.normal = asMemberPtr<CopyCtor>((void *)ptr);
			return result;
		}
	};

	struct CppTypeInfoTable {
		unsigned count;
		DWORD infoOffset[1];

		const CppTypeInfo *info(HINSTANCE instance) const {
			return (const CppTypeInfo *)(size_t(instance) + size_t(infoOffset[0]));
		}
	};

	struct CppExceptionType {
		unsigned flags;
		DWORD dtorOffset;
		DWORD handlerOffset; // void (*handler)()
		DWORD tableOffset;

		Dtor dtor(HINSTANCE instance) const {
			return asMemberPtr<Dtor>((void *)(size_t(instance) + size_t(dtorOffset)));
		}
		const CppTypeInfoTable *table(HINSTANCE instance) const {
			return (const CppTypeInfoTable *)(size_t(instance) + size_t(tableOffset));
		}
	};

	static const CppTypeInfo *getCppTypeInfo(const CppExceptionType *t, HINSTANCE instance) {
		const CppTypeInfo *info = t->table(instance)->info(instance);
		assert(info);
		return info;
	}

	static void copyException(void *to, void *from, const CppTypeInfo *info, HINSTANCE instance) {
		CppCopyCtor copy = info->copyCtor(instance);

		bool useCopyCtor = (info->flags & simpleType) == 0;
		useCopyCtor &= copy.normal != null;

		if (useCopyCtor) {
			DummyException *p = (DummyException *)to;
			if (info->flags & virtualBaseType)
				(p->*(copy.virtualCtor))(from, to);
			else
				(p->*(copy.normal))(from);
		} else {
			memmove(to, from, info->size);
		}
	}

	FutureBase::ExceptionPtr::ExceptionPtr() : data(null), type(null), instance(null) {}

	FutureBase::ExceptionPtr::~ExceptionPtr() {
		clear();
	}

	void FutureBase::ExceptionPtr::clear() {
		if (data) {
			Dtor dtor = type->dtor(instance);
			if (dtor) {
				DummyException *p = (DummyException *)data;
				(p->*dtor)();
			}
			free(data);
		}
		data = null;
		type = null;
		instance = null;
	}

	void FutureBase::ExceptionPtr::rethrow() {
		// It is safe to keep stuff at the stack here. catch-blocks are executed
		// on top of this stack, ie this stack frame will not be reclaimed until
		// the catch-blocks have been executed.
		const CppTypeInfo *info = getCppTypeInfo(type, instance);
		void *dst = _alloca(info->size);
		copyException(dst, data, info, instance);
		ULONG_PTR args[cppExceptionParams];
		args[0] = cppExceptionMagic;
		args[1] = (ULONG_PTR)dst;
		args[2] = (ULONG_PTR)type;
		if (cppExceptionParams >= 4)
			args[3] = (ULONG_PTR)instance;
		RaiseException(cppExceptionCode, EXCEPTION_NONCONTINUABLE, cppExceptionParams, args);
	}

	void FutureBase::ExceptionPtr::rethrowPtr(PtrThrowable *&store) {
		// Make a copy on the stack so that we don't accidentally point somewhere unsafe.
		PtrThrowable *copy = store;
		ULONG_PTR args[cppExceptionParams];
		args[0] = cppExceptionMagic;
		args[1] = (ULONG_PTR)&copy;
		args[2] = (ULONG_PTR)type;
		if (cppExceptionParams >= 4)
			args[3] = (ULONG_PTR)instance;
		RaiseException(cppExceptionCode, EXCEPTION_NONCONTINUABLE, cppExceptionParams, args);
	}

	void FutureBase::ExceptionPtr::set(void *src, const CppExceptionType *type, HINSTANCE instance) {
		clear();

		this->type = type;
		this->instance = instance;
		const CppTypeInfo *ti = getCppTypeInfo(type, instance);
		data = malloc(ti->size);
		try {
			copyException(data, src, ti, instance);
		} catch (...) {
			free(data);
			throw;
		}
	}

	void FutureBase::ExceptionPtr::setPtr(void *src, const CppExceptionType *type, HINSTANCE instance, PtrThrowable *&store) {
		clear();

		// Indicate that we need an external pointer for this!
		data = null;
		this->type = type;
		this->instance = instance;
		store = *(PtrThrowable **)src;
	}

	static bool isCppException(EXCEPTION_RECORD *record) {
		return record
			&& record->ExceptionCode == cppExceptionCode
			&& record->NumberParameters == cppExceptionParams
			&& record->ExceptionInformation[0] == cppExceptionMagic;
	}

	int FutureBase::filter(EXCEPTION_POINTERS *info, bool pointer) {
		EXCEPTION_RECORD *record = info->ExceptionRecord;

		if (!isCppException(record)) {
			WARNING(L"Thrown exception is not a C++ exception!");
			return EXCEPTION_CONTINUE_SEARCH;
		}

		if (!record->ExceptionInformation[2]) {
			// It was a re-throw. We need to go look up the exception record from tls!
			byte *base = (byte *)_errno();
			record = *(EXCEPTION_RECORD **)(base + exceptionInfoOffset);
		}

		if (!isCppException(record)) {
			WARNING(L"Thrown exception is a C++ exception, but stored is not.");
		}
		assert(record->ExceptionInformation[2]);

		HINSTANCE instance = 0;
#ifdef X64
		instance = (HINSTANCE)record->ExceptionInformation[3];
#endif

		if (pointer)
			exceptionData.setPtr((void *)record->ExceptionInformation[1],
								(const CppExceptionType *)record->ExceptionInformation[2],
								instance,
								ptrException);
		else
			exceptionData.set((void *)record->ExceptionInformation[1],
							(const CppExceptionType *)record->ExceptionInformation[2],
							instance);

		return EXCEPTION_EXECUTE_HANDLER;
	}

	void FutureBase::saveError() {
		__try {
			throw;
		} __except (filter(GetExceptionInformation(), false)) {
		}
	}

	void FutureBase::savePtrError(const PtrThrowable *) {
		__try {
			throw;
		} __except (filter(GetExceptionInformation(), true)) {
		}
	}


	void FutureBase::throwError() {
		exceptionData.rethrow();
	}

	void FutureBase::throwPtrError(InterceptFn fn, void *env) {
		if (fn) {
			PtrThrowable *modified = (*fn)(ptrException, env);
			exceptionData.rethrowPtr(modified);
		} else {
			exceptionData.rethrowPtr(ptrException);
		}
	}

	void FutureBase::cleanError() {}

#elif defined(POSIX)

	void FutureBase::throwError() {
		void *data = exceptionData;
		std::rethrow_exception(*(std::exception_ptr *)data);
	}

	extern "C" void *__cxa_allocate_exception(size_t size);
	extern "C" void __cxa_throw(void *exception, std::type_info *type, void (*dtor)(void *));

	void FutureBase::throwPtrError(InterceptFn fn, void *env) {
		void *mem = __cxa_allocate_exception(sizeof(void *));
		if (fn) {
			*(void **)mem = (*fn)(ptrException, env);
		} else {
			*(void **)mem = ptrException;
		}
		__cxa_throw(mem, (std::type_info *)exceptionData[0], null);
	}

	void FutureBase::saveError() {
		new (exceptionData) std::exception_ptr(std::current_exception());
	}

	void FutureBase::savePtrError(const PtrThrowable *) {
		std::exception_ptr ptr = std::current_exception();

		// Store the exception type.
		exceptionData[0] = (size_t)ptr.__cxa_exception_type();

		// Store the exception itself.
		// Note: std::exception_ptr is just a pointer to the thrown object, which is a pointer in this case.
		void *internal = *(void **)&ptr;
		ptrException = *(PtrThrowable **)internal;
	}

	void FutureBase::cleanError() {
		if (resultPosted == resultError) {
			// Destroy it properly.
			void *data = exceptionData;
			std::exception_ptr *ePtr = (std::exception_ptr *)data;
			ePtr->~exception_ptr();
		}
	}

#else

	void FutureBase::throwError() {
		std::rethrow_exception(exceptionData);
	}

	int FutureBase::filter(EXCEPTION_POINTERS *ptrs, InterceptFn fn, void *env) {
		// Modify the thrown object in-flight. This should be fine as we are the first one to see
		// the object, and since the object is generally copied to the stack before being thrown (if
		// not, we don't care about the old value anyway, but could be problematic if multiple
		// threads re-throw simultaneously).
		// It seems like the exception is copied to the stack, but this needs more investigation.
		PtrThrowable **ePtr = (PtrThrowable **)ptrs->ExceptionRecord->ExceptionInformation[1];
		if (fn) {
			*ePtr = (*fn)(ptrException, env);
		} else {
			*ePtr = ptrException;
		}
		return EXCEPTION_CONTINUE_SEARCH;
	}

	void FutureBase::throwPtrError(InterceptFn fn, void *env) {
		__try {
			throwError();
		} __except (filter(GetExceptionInformation(), fn, env)) {
		}
	}

	void FutureBase::saveError() {
		exceptionData = std::current_exception();
	}

	void FutureBase::savePtrError(const PtrThrowable *exception) {
		exceptionData = std::current_exception();
		ptrException = const_cast<PtrThrowable *>(exception);
	}

	void FutureBase::cleanError() {
		ptrException = null;
		exceptionData = std::exception_ptr();
	}


#endif

}
