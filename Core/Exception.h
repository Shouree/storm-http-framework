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
		void STORM_FN deepCopy(CloneEnv *env);

		// Get the message.
		Str *STORM_FN message() const;

		// Append the message to a string buffer.
		virtual void STORM_FN message(StrBuf *to) const ABSTRACT;

		// Collected stack trace, if any.
		StackTrace stackTrace;

		// Convenience function to call to collect a stack trace. Intended to be called from the
		// constructor of derived exceptions who wish to store a stack trace. This is not done by
		// default, as it is fairly expensive, and not beneficial for all exceptions (e.g. syntax
		// errors).
		void STORM_FN saveTrace();

	protected:
		// Regular to string implementation.
		virtual void STORM_FN toS(StrBuf *to) const;
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
	 * Internal error.
	 */
	class EXCEPTION_EXPORT InternalError : public Exception {
		STORM_EXCEPTION;
	public:
		InternalError(const wchar *msg);
		STORM_CTOR InternalError(Str *msg);

		virtual void STORM_FN message(StrBuf *to) const;
	private:
		Str *msg;
	};


	/**
	 * Runtime errors.
	 */
	class EXCEPTION_EXPORT RuntimeError : public Exception {
		STORM_EXCEPTION;
	public:
		RuntimeError(const wchar *msg);
		STORM_CTOR RuntimeError(Str *msg);

		virtual void STORM_FN message(StrBuf *to) const;
	private:
		Str *msg;
	};


	/**
	 * Calling an abstract function.
	 */
	class EXCEPTION_EXPORT AbstractFnCalled : public RuntimeError {
	public:
		AbstractFnCalled(const wchar *name);
		STORM_CTOR AbstractFnCalled(Str *msg);
		virtual void STORM_FN message(StrBuf *to) const;
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
