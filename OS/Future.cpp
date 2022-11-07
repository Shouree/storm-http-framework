#include "stdafx.h"
#include "Future.h"
#include "PtrThrowable.h"

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
		} catch (const PtrThrowable *) {
			// Save the exception pointer directly. It may be GC:d.
			nat p = atomicCAS(resultPosted, resultEmpty, resultErrorPtr);
			assert(p == resultEmpty, L"A future may not be used more than once! this=" + ::toHex(this));
			(void)p; // Avoid warning in release mode.
			savePtrError();
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

#ifndef X86
#error "Only X86 is supported now. Please revise the code or use a compiler with support for exception_ptr"
#endif

	/**
	 * Implementation more or less copied from Boost exception_ptr.
	 */

	const nat cppExceptionCode = 0xE06D7363;
	const nat cppExceptionMagic = 0x19930520;
	const nat cppExceptionParams = 3;
#if _MSC_VER==1310
	const nat exceptionInfoOffset = 0x74;
#elif (_MSC_VER == 1400 || _MSC_VER == 1500)
	const nat exceptionInfoOffset = 0x80;
#else
#error "Unknown MSC version."
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

	struct CppTypeInfo {
		unsigned flags;
		std::type_info * typeInfo;
		int thisOffset;
		int vbaseDescr;
		int vbaseOffset;
		unsigned long size;
		CppCopyCtor copyCtor;
	};

	struct CppTypeInfoTable {
		unsigned count;
		const CppTypeInfo *info[1];
	};

	struct CppExceptionType {
		unsigned flags;
		Dtor dtor;
		void(*handler)();
		const CppTypeInfoTable *table;
	};

	static const CppTypeInfo *getCppTypeInfo(const CppExceptionType *t) {
		const CppTypeInfo *info = t->table->info[0];
		assert(info);
		return info;
	}

	static void copyException(void *to, void *from, const CppTypeInfo *info) {
		bool copyCtor = (info->flags & simpleType) == 0;
		copyCtor &= info->copyCtor.normal != null;

		if (copyCtor) {
			DummyException *p = (DummyException *)to;
			if (info->flags & virtualBaseType)
				(p->*(info->copyCtor.virtualCtor))(from, to);
			else
				(p->*(info->copyCtor.normal))(from);
		} else {
			memmove(to, from, info->size);
		}
	}

	FutureBase::ExceptionPtr::ExceptionPtr() : data(null), type(null) {}

	FutureBase::ExceptionPtr::~ExceptionPtr() {
		clear();
	}

	void FutureBase::ExceptionPtr::clear() {
		if (data) {
			if (type->dtor) {
				DummyException *p = (DummyException *)data;
				(p->*(type->dtor))();
			}
			free(data);
		}
		data = null;
		type = null;
	}

	void FutureBase::ExceptionPtr::rethrow() {
		// It is safe to keep stuff at the stack here. catch-blocks are executed
		// on top of this stack, ie this stack frame will not be reclaimed until
		// the catch-blocks have been executed.
		const CppTypeInfo *info = getCppTypeInfo(type);
		void *dst = _alloca(info->size);
		copyException(dst, data, info);
		ULONG_PTR args[cppExceptionParams];
		args[0] = cppExceptionMagic;
		args[1] = (ULONG_PTR)dst;
		args[2] = (ULONG_PTR)type;
		RaiseException(cppExceptionCode, EXCEPTION_NONCONTINUABLE, cppExceptionParams, args);
	}

	void FutureBase::ExceptionPtr::rethrowPtr(PtrThrowable *&store) {
		// Make a copy on the stack so that we don't accidentally point somewhere unsafe.
		PtrThrowable *copy = store;
		ULONG_PTR args[cppExceptionParams];
		args[0] = cppExceptionMagic;
		args[1] = (ULONG_PTR)&copy;
		args[2] = (ULONG_PTR)type;
		RaiseException(cppExceptionCode, EXCEPTION_NONCONTINUABLE, cppExceptionParams, args);
	}

	void FutureBase::ExceptionPtr::set(void *src, const CppExceptionType *type) {
		clear();

		this->type = type;
		const CppTypeInfo *ti = getCppTypeInfo(type);
		data = malloc(ti->size);
		try {
			copyException(data, src, ti);
		} catch (...) {
			free(data);
			throw;
		}
	}

	void FutureBase::ExceptionPtr::setPtr(void *src, const CppExceptionType *type, PtrThrowable *&store) {
		clear();

		// Indicate that we need an external pointer for this!
		data = null;
		this->type = type;
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
			WARNING(L"Not a C++ exception!");
			return EXCEPTION_CONTINUE_SEARCH;
		}

		if (!record->ExceptionInformation[2]) {
			// It was a re-throw. We need to go look up the exception record from tls!
			byte *t = (byte *)_errno();
			record = *(EXCEPTION_RECORD **)(t + exceptionInfoOffset);
			//record = *reinterpret_cast<EXCEPTION_RECORD **>(reinterpret_cast<char *>(_errno()) + exceptionInfoOffset);
		}

		if (!isCppException(record)) {
			WARNING(L"Not a C++ exception!");
		}
		assert(record->ExceptionInformation[2]);

		if (pointer)
			exceptionData.setPtr((void *)record->ExceptionInformation[1],
								(const CppExceptionType *)record->ExceptionInformation[2],
								ptrException);
		else
			exceptionData.set((void *)record->ExceptionInformation[1],
							(const CppExceptionType *)record->ExceptionInformation[2]);

		return EXCEPTION_EXECUTE_HANDLER;
	}

	void FutureBase::saveError() {
		__try {
			throw;
		} __except (filter(GetExceptionInformation(), false)) {
		}
	}

	void FutureBase::savePtrError() {
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

	void FutureBase::savePtrError() {
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

	void FutureBase::savePtrError() {
		exceptionData = std::current_exception();
	}

	void FutureBase::cleanError() {}


#endif

}
