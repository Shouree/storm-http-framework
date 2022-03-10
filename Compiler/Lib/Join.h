#pragma once
#include "Compiler/Template.h"

namespace storm {
	STORM_PKG(core.lang);

	/**
	 * Storm version of the Join interface.
	 *
	 * All in all, allows writing one of the following, where "x" is an array of some object:
	 * - to << join(x) // Same as 'join(x, "")'
	 * - to << join(x, ", ")
	 * - to << join(x, <function>)
	 * - to << join(x, ", ", <function>)
	 */
	class Join : public Object {
		STORM_CLASS;
	public:
		// Create. Separator and may be null.
		Join(ArrayBase *array, MAYBE(Str *) separator);
		Join(ArrayBase *array, MAYBE(Str *) separator, MAYBE(FnBase *) lambda, void *lambdaCall, os::CallThunk thunk);

	protected:
		// Output. This is where the actual 'join' happens.
		virtual void STORM_FN toS(StrBuf *to) const;

	private:
		// Array.
		ArrayBase *array;

		// Separator.
		MAYBE(Str *) separator;

		// Lambda, if any.
		MAYBE(FnBase *) lambda;

		// The 'call' function in the lambda function. Note: It would be better to store a Ref to
		// this, but since we expect Join objects to be short-lived this is OK.
		UNKNOWN(PTR_GC) void *lambdaCall;

		// Get the call thunk used to call the lambda function.
		UNKNOWN(PTR_GC) os::CallThunk thunk;
	};

	// Helper we need a reference to.
	Join *CODECALL createJoin(ArrayBase *array, Str *separator, FnBase *lambda, void *lambdaCall, os::CallThunk thunk);

	/**
	 * Template for creating "join" implementations.
	 */
	class JoinTemplate : public Template {
		STORM_CLASS;
	public:
		// Create.
		STORM_CTOR JoinTemplate();

		// Create things.
		virtual MAYBE(Named *) STORM_FN generate(SimplePart *part);

	private:
		// Created instances of the Join class, so that we can re-use old instances in similar
		// situations. The key here is the type in the array, i.e. the parameter that is passed to
		// the function parameter.

		// Two-parameter versions.
		Map<Type *, Named *> *created2;

		// Three-parameter versions.
		Map<Type *, Named *> *created3;
	};

}
