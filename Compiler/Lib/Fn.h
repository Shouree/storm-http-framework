#pragma once
#include "ValueArray.h"
#include "Type.h"
#include "Core/Variant.h"

namespace storm {
	STORM_PKG(core.lang);

	// Create the Fn type.
	Type *createFn(Str *name, ValueArray *params);

	/**
	 * Type for function pointers.
	 */
	class FnType : public Type {
		STORM_CLASS;
	public:
		// Create.
		STORM_CTOR FnType(Str *name, ValueArray *params);

		// Get the result type of this function. Convenience for examining the first parameter manually.
		Value STORM_FN result();

		// Get the parameters of this function. Convenience for examining the parameters manually.
		Array<Value> *STORM_FN parameters();

	protected:
		// Load.
		virtual Bool STORM_FN loadAll();

	private:
		// Generate the code for calling the function.
		CodeGen *CODECALL callCode();

		// Generate code returned by 'rawCall'.
		CodeGen *CODECALL rawCallCode();

		// Create the contents of 'rawCall'.
		code::RefSource *createRawCall();

		// Thunk used for function calls.
		code::RefSource *thunk;

		// Code used by 'rawCall'.
		code::RefSource *rawCall;
	};

	// Find the function type.
	Type *fnType(Array<Value> *params) ON(Compiler);

	// Create a function pointer to a function.
	FnBase *STORM_FN pointer(Function *target) ON(Compiler);

	// Create a function pointer to a function. Binds `thisPtr` to the first parameter.
	FnBase *STORM_FN pointer(Function *target, Object *thisPtr) ON(Compiler);

	// Create a function pointer to a function. Binds `thisPtr` to the first parameter.
	FnBase *STORM_FN pointer(Function *target, TObject *thisPtr) ON(Compiler);

	// Dynamic calls to functions by using Variants. Checks the types of parameters before
	// performing the call.
	Variant STORM_FN dynamicCall(Function *function, Array<Variant> *params) ON(Compiler);

	// Low-level functionality required by generated machine code.
	void CODECALL fnCallRaw(FnBase *b, void *output, os::CallThunk thunk, void **params, TObject *first);

	// Low-level creation from generated code.
	FnBase *CODECALL fnCreateRaw(Type *type, code::RefSource *to, Thread *thread, RootObject *thisPtr, Bool memberFn);

}
