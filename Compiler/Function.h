#pragma once
#include "Named.h"
#include "RunOn.h"
#include "Code.h"
#include "NamedSource.h"
#include "Code/Reference.h"
#include "Core/Future.h"
#include "OS/Future.h"

namespace storm {
	STORM_PKG(core.lang);

	/**
	 * Flags applicable to functions.
	 */
	enum FnFlags {
		// Nothing special about this function.
		STORM_NAME(fnNone, none) = 0x00,

		// This function is pure, meaning that it has no side effects and accesses no global
		// data. This means that two calls with the same parameters will always yield the same
		// results, which in turn means that the result can be cached or computed compile-time.
		STORM_NAME(fnPure, pure) = 0x01,

		// This function is a constructor suitable for automatic type casts. Only relevant for constructors.
		STORM_NAME(fnAutoCast, autoCast) = 0x02,

		// This is a setter function, and as such we want to be able to use the function as the
		// target of an assignment.
		STORM_NAME(fnAssign, assign) = 0x04,

		// This function is final.
		STORM_NAME(fnFinal, final) = 0x08,

		// This function is abstract (ie. has to be overridden).
		STORM_NAME(fnAbstract, abstract) = 0x10,

		// This function is expected to override something.
		STORM_NAME(fnOverride, override) = 0x20,

		// This function is static (ie. 'isMember' never returns true).
		STORM_NAME(fnStatic, static) = 0x40,
	};

	BITMASK_OPERATORS(FnFlags);

	/**
	 * Describes a function in the compiler, either a member function or a free function. In the
	 * case of a member function, the 'this' pointer is explicitly stated as the first parameter.
	 *
	 * See 'isMember' for details on which functions are considered member functions and nonmember
	 * functions.
	 */
	class Function : public Named {
		STORM_CLASS;
	public:
		// Create a function.
		STORM_CTOR Function(Value result, Str *name, Array<Value> *params);
		STORM_CTOR Function(SrcPos pos, Value result, Str *name, Array<Value> *params);

		// Return value of the function.
		const Value result;

		// Get the code for this function. Do not assume this pointer is static! For anything else
		// than making a call in the same function, use a proper reference instead.
		const void *pointer();

		// Get a reference to the code for this function.
		code::Ref STORM_FN ref();

		// Get a reference directly to the underlying function, bypassing any vtable lookups or
		// similar. Mainly used for implementing super-calls.
		code::Ref STORM_FN directRef();

		// Is this a member function?
		// A function is considered a member function if it is located inside a function, the
		// first parameter of the function matches that type, *and* it is not static.
		Bool STORM_FN isMember();

		// Which thread shall we run on?
		virtual RunOn STORM_FN runOn() const;

		// Set where to run.
		void STORM_FN runOn(NamedThread *thread);

		// Get declared "run on". Note: this is inherited from parent types in certain situations.
		inline MAYBE(NamedThread *) STORM_FN declaredThread() const { return runOnThread; }

		// Is this function 'pure'? Ie. are we sure that this function does not produce any side effects?
		// For constructors and destructors, this means that we can ignore calling the function when the
		// compiler deems it unneccessary. For other functions, it means that the compiler may choose to
		// perform constant folding or common subexpression elimination through this function safely.
		virtual Bool STORM_FN pure() const;

		// Add a flag to the set of flags.
		Function *STORM_FN make(FnFlags flag);

		// Helper for C++.
		inline Function *STORM_FN makePure() { return make(fnPure); }
		inline Function *STORM_FN makeAutoCast() { return make(fnAutoCast); }

		// Helpers for the grammar.
		inline Function *STORM_FN makeAbstract() { return make(fnAbstract); }
		inline Function *STORM_FN makeFinal() { return make(fnFinal); }
		inline Function *STORM_FN makeOverride() { return make(fnOverride); }

		// Get function flags.
		FnFlags STORM_FN fnFlags() const;

		// Output.
		virtual void STORM_FN toS(StrBuf *to) const;

		/**
		 * Code.
		 */

		// Set the code. Note: this safely updates all existing references to the code.
		void STORM_FN setCode(Code *code);

		// Get the code in use.
		MAYBE(Code *) STORM_FN getCode() const;

		// Set lookup code. A default one is provided, use null to restore it.
		void STORM_FN setLookup(MAYBE(Code *) lookup);

		// Get the lookup currently in use.
		MAYBE(Code *) STORM_FN getLookup() const;

		// Forcefully compile this function.
		void STORM_FN compile();

		// Indicate that we don't need the source code for this function anymore.
		void STORM_FN discardSource();

		// Get the original C++ function pointer. If none exists, returns
		// 'directRef->address()'. Intended to be used to find vtable slots for this function.
		virtual const void *originalPtr();

		// Get a reference to the call thunk for this function. This allows calling the function
		// through a FnCall object without actually knowing the type of all parameters and/or the
		// return value.
		code::Ref STORM_FN thunkRef();

		// Get a properly casted call thunk.
		inline os::CallThunk callThunk() { return (os::CallThunk)thunkRef().address(); }

		/**
		 * Generate calls to this function.
		 */

		// Output code to find the Thread we want to run on. Always returns a borrowed reference.
		// Does not generate any meaningful result unless 'runOn' returns a state other than 'any'.
		virtual code::Var STORM_FN findThread(CodeGen *to, Array<code::Operand> *params);

		// Figure out which kind of call we shall use and do the right thing based on what the
		// 'runOn' member in 'to' is indicating. Always uses lookup. Parameters passed are as
		// follows: If the type of the parameter is a class, the parameter is a pointer to the
		// class, if the type is a value, the parameter is a pointer to the value or a variable
		// containing the value; the value will be copied to the function. If the type is a built in
		// type, the parameter is the actual value.
		void STORM_FN autoCall(CodeGen *to, Array<code::Operand> *params, CodeResult *result);

		// Same as 'autoCall', but assumes 'result' is a pointer to where the result is to be stored.
		void STORM_FN autoCallRef(CodeGen *to, Array<code::Operand> *params, code::Operand result);

		// Generate code for this function call, assuming we are performing the call the same thread as the
		// currently running thread. If 'useLookup' is false, we will not use the lookup function (ie VTables).
		void STORM_FN localCall(CodeGen *to, Array<code::Operand> *params, CodeResult *result, Bool useLookup);

		// Same as 'localCall', but assumes 'result' is a pointer to where the result is to be stored.
		void STORM_FN localCallRef(CodeGen *to, Array<code::Operand> *params, code::Operand result, Bool useLookup);

		// Generate code for this function call, assuming we want to run on a different thread.
		void STORM_FN threadCall(CodeGen *to, Array<code::Operand> *params, CodeResult *result, code::Operand thread);

		// Same as 'threadCall', but assumes 'result' is a pointer to where the result is to be stored.
		void STORM_FN threadCallRef(CodeGen *to, Array<code::Operand> *params, code::Operand result, code::Operand thread);

		// Generate code for this function call, asuming we want to do an async code. Examines the
		// thread "affinity" of this function and clones parameters whenever needed. Like "autoCall"
		// but returns a Future. The variant with "id" returns the newly created thread ID there.
		void STORM_FN asyncAutoCall(CodeGen *to, Array<code::Operand> *params, CodeResult *result);
		void STORM_FN asyncAutoCall(CodeGen *to, Array<code::Operand> *params, CodeResult *result, CodeResult *id);

		// Generate code for this function call, but assume we never need to clone our
		// parameters. Much like "localCall", but returns a Future. Always spawns the new UThread on
		// the current Thread.
		void STORM_FN asyncLocalCall(CodeGen *to, Array<code::Operand> *params, CodeResult *result);
		void STORM_FN asyncLocalCall(CodeGen *to, Array<code::Operand> *params, CodeResult *result, CodeResult *id);

		// Generate code for this function call, assuming we want to run on a different thread,
		// returning a Future object. If 'thread' is specified, it is used for functions which have
		// not been specified to run on a particular thread.
		void STORM_FN asyncThreadCall(CodeGen *to, Array<code::Operand> *params, CodeResult *result);
		void STORM_FN asyncThreadCall(CodeGen *to, Array<code::Operand> *params, CodeResult *result, code::Operand thread);
		void STORM_FN asyncThreadCall(CodeGen *to, Array<code::Operand> *params, CodeResult *future, CodeResult *id);
		void STORM_FN asyncThreadCall(CodeGen *to, Array<code::Operand> *params, CodeResult *future, code::Operand thread, CodeResult *id);

	private:
		// Two references. One is for the lookup function, and one is for the actual code to be
		// executed. Initialized when needed due to lack of information in the ctor.
		code::RefSource *lookupRef;
		code::RefSource *codeRef;

		// Stored code.
		Code *lookup;
		Code *code;

		// Thread we shall be running on:
		NamedThread *runOnThread;

		// Flags for this function.
		FnFlags myFlags;

		// Initialize references.
		void initRefs();

		// Helper for calling this function from another thread.
		code::RefSource *threadThunkRef;

		// Get the thread thunk, generates it if needed. Returns null if no thunk is needed. This
		// may be used to call the function through a generic FnParams object without knowing the
		// types of all parameters and/or the return value.
		code::RefSource *STORM_FN threadThunk();

		// Generate code for a direct function call.
		void localCall(CodeGen *to, Array<code::Operand> *params, CodeResult *result, code::Ref ref);
		void localCallRef(CodeGen *to, Array<code::Operand> *params, code::Operand result, code::Ref ref);

		// Helper for the async function call. 'thread' is used if we don't have a better opinion of what to use.
		void asyncCall(CodeGen *to, Array<code::Operand> *params, code::Operand thread,
					CodeResult *future, CodeResult *id, bool copy);

		// Helper for the async function call, in the special case where the function returns void
		// and the caller is not interested in the result (but possibly it ID).
		void asyncCallVoid(CodeGen *to, Array<code::Operand> *params, code::Operand thread,
						CodeResult *id, bool copy);

		// Add parameters for the function call.
		void addParams(CodeGen *to, Array<code::Operand> *params);
		void addParam(CodeGen *to, Array<code::Operand> *params, nat id);
	};

	/**
	 * Function in C++. Also contains the original pointer to the function (ie. before
	 * de-virtualization) so that we can find the vtable slot easier and with higher certainty.
	 */
	class CppMemberFunction : public Function {
		STORM_CLASS;
	public:
		// Create.
		CppMemberFunction(Value result, Str *name, Array<Value> *params, const void *original);

		// Get the original pointer.
		virtual const void *originalPtr();

	private:
		// Original function pointer.
		const void *original;
	};


	// Conveniently create various kind of functions.
	Function *inlinedFunction(Engine &e, Value result, const wchar *name, Array<Value> *params, Fn<void, InlineParams> *fn);
	Function *nativeFunction(Engine &e, Value result, const wchar *name, Array<Value> *params, const void *fn);
	Function *nativeEngineFunction(Engine &e, Value result, const wchar *name, Array<Value> *params, const void *fn);
	Function *lazyFunction(Engine &e, Value result, const wchar *name, Array<Value> *params, Fn<CodeGen *> *generate);
	Function *dynamicFunction(Engine &e, Value result, const wchar *name, Array<Value> *params, code::Listing *src);


	// Helpers used by the generated code.
	void spawnThreadResult(const void *fn, bool member, os::CallThunk thunk, void **params, void *result, Thread *on);
	void spawnThreadFuture(const void *fn, bool member, os::CallThunk thunk, void **params, FutureBase *result, Thread *on);
	Word spawnThreadId(const void *fn, bool member, os::CallThunk thunk, void **params, FutureBase *result, Thread *on);
	void spawnThreadVoid(const void *fn, bool member, os::CallThunk thunk, void **params, Thread *on);
	Word spawnThreadVoidId(const void *fn, bool member, os::CallThunk thunk, void **params, Thread *on);

}
