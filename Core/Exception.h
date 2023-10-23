#pragma once
#include "Str.h"
#include "StrBuf.h"
#include "StackTrace.h"

namespace storm {
	STORM_PKG(core);

	/**
	 * Base class for all exceptions in Storm.
	 */
	class EXCEPTION_EXPORT Exception : public Object {
		STORM_EXCEPTION_BASE;
	public:
		// Create.
		STORM_CTOR Exception();

		// Copy.
		Exception(const Exception &o);

		// Deep copy.
		virtual void STORM_FN deepCopy(CloneEnv *env);

		// Retrieve the message in the exception. This function simply calls `message(StrBuf)` to generate the message.
		Str *STORM_FN message() const;

		// Create the message by appending it to the string buffer.
		virtual void STORM_FN message(StrBuf *to) const ABSTRACT;

		// Collected stack trace, if any.
		StackTrace stackTrace;

		// Convenience function to call to collect a stack trace. Intended to be called from the
		// constructor of derived exceptions who wish to store a stack trace. This is not done by
		// default, as it is fairly expensive, and not beneficial for all exceptions (e.g. syntax
		// errors). Some categories of exceptions do, however, capture stack traces automatically.
		// Returns `this` to allow chaining it after creation.
		Exception *STORM_FN saveTrace();

		// For providing a context in exception handlers.
		Exception *saveTrace(void *context);

	protected:
		// Regular to string implementation.
		virtual void STORM_FN toS(StrBuf *to) const;
	};


	/**
	 * Runtime errors from various parts of the system.
	 */
	class EXCEPTION_EXPORT RuntimeError : public Exception {
		STORM_EXCEPTION;
	public:
		STORM_CTOR RuntimeError();
	};


	/**
	 * Exception thrown by the garbage collector.
	 *
	 * The most common type of error is an out of memory error, but others may occur.
	 */
	class EXCEPTION_EXPORT GcError : public RuntimeError {
		STORM_EXCEPTION;
	public:
		// Create, basic C-string to avoid memory allocations.
		GcError(const wchar *msg);

		virtual void STORM_FN message(StrBuf *to) const;

	private:
		// The message.
		const wchar *msg;
	};


	/**
	 * Arithmetic errors. Thrown by numerical operations when an error occurs.
	 */
	class EXCEPTION_EXPORT NumericError : public RuntimeError {
		STORM_EXCEPTION;
	public:
		// Create.
		STORM_CTOR NumericError();
	};


	/**
	 * Division by zero. Thrown on integer divisions by zero. Floating point numbers generate
	 * infinity/nan in many cases instead.
	 */
	class EXCEPTION_EXPORT DivisionByZero : public NumericError {
		STORM_EXCEPTION;
	public:
		// Create.
		STORM_CTOR DivisionByZero();
		DivisionByZero(void *context);

		// Message.
		virtual void STORM_FN message(StrBuf *to) const;
	};


	/**
	 * Access violation. Thrown when an invalid address has been accessed.
	 */
	class EXCEPTION_EXPORT MemoryAccessError : public RuntimeError {
		STORM_EXCEPTION;
	public:
		enum Type {
			notMapped,
			invalidAccess,
			invalidAlignment,
			kernel,
		};

		// Create. If 'mapped' is true, then the address is mapped, but the access type was invalid.
		STORM_CTOR MemoryAccessError(Word address, Type type);
		MemoryAccessError(Word address, Type type, void *context);

		// Message.
		virtual void STORM_FN message(StrBuf *to) const;

	private:
		// Faulting address.
		Word address;

		// Type of fault.
		Type type;
	};


	/**
	 * Generic exceptions.
	 */
	class EXCEPTION_EXPORT NotSupported : public Exception {
		STORM_EXCEPTION;
	public:
		NotSupported(const wchar *msg);
		STORM_CTOR NotSupported(Str *msg);

		virtual void STORM_FN message(StrBuf *to) const;

	private:
		Str *msg;
	};


	/**
	 * Invalid usage.
	 */
	class EXCEPTION_EXPORT UsageError : public Exception {
		STORM_EXCEPTION;
	public:
		UsageError(const wchar *msg);
		STORM_CTOR UsageError(Str *msg);

		virtual void STORM_FN message(StrBuf *to) const;

	private:
		Str *msg;
	};


	/**
	 * Internal error.
	 */
	class EXCEPTION_EXPORT InternalError : public RuntimeError {
		STORM_EXCEPTION;
	public:
		InternalError(const wchar *msg);
		STORM_CTOR InternalError(Str *msg);

		virtual void STORM_FN message(StrBuf *to) const;
	private:
		Str *msg;
	};


	/**
	 * Calling an abstract function.
	 */
	class EXCEPTION_EXPORT AbstractFnCalled : public RuntimeError {
		STORM_EXCEPTION;
	public:
		AbstractFnCalled(const wchar *name);
		STORM_CTOR AbstractFnCalled(Str *msg);

		virtual void STORM_FN message(StrBuf *to) const;

	private:
		Str *name;
	};


	/**
	 * Custom exception for strings. Cannot be in Str.h due to include cycles.
	 */
	class EXCEPTION_EXPORT StrError : public Exception {
		STORM_EXCEPTION;
	public:
		StrError(const wchar *msg);
		STORM_CTOR StrError(Str *msg);
		virtual void STORM_FN message(StrBuf *to) const;
	private:
		Str *msg;
	};


	/**
	 * Exception thrown from the map.
	 */
	class EXCEPTION_EXPORT MapError : public Exception {
		STORM_EXCEPTION;
	public:
		MapError(const wchar *msg);
		STORM_CTOR MapError(Str *msg);
		virtual void STORM_FN message(StrBuf *to) const;
	private:
		Str *msg;
	};


	/**
	 * Exception thrown from the set.
	 */
	class EXCEPTION_EXPORT SetError : public Exception {
		STORM_EXCEPTION;
	public:
		SetError(const wchar *msg);
		STORM_CTOR SetError(Str *msg);
		virtual void STORM_FN message(StrBuf *to) const;
	private:
		Str *msg;
	};


	/**
	 * Custom error type for arrays.
	 */
	class EXCEPTION_EXPORT ArrayError : public Exception {
		STORM_EXCEPTION;
	public:
		STORM_CTOR ArrayError(Nat id, Nat count);
		STORM_CTOR ArrayError(Nat id, Nat count, Str *msg);
		virtual void STORM_FN message(StrBuf *to) const;
	private:
		Nat id;
		Nat count;
		MAYBE(Str *) msg;
	};


}
