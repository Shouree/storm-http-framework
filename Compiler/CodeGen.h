#pragma once
#include "Code/Listing.h"
#include "Code/Binary.h"
#include "RunOn.h"

namespace storm {
	STORM_PKG(core.lang);

	class CodeGen;
	class Function;

	/**
	 * Information about a created variable. It contains:
	 * - the variable itself
	 * - if the variable needs a specific part for itself
	 *
	 * In the case of values, they need to be allocated in a separate part so that their destructor
	 * is not executed too early in case of an exception.
	 */
	class VarInfo {
		STORM_VALUE;
	public:
		// Create.
		STORM_CTOR VarInfo();

		// Create, assume we do not need anything special.
		STORM_CTOR VarInfo(code::Var v);

		// Create, set flags.
		STORM_CTOR VarInfo(code::Var v, Bool needsActivation);

		// Variable.
		code::Var v;

		// Does this variable need to be activated after it is created?
		Bool needsActivation;

		// Do anything needed when the variable has been created.
		void STORM_FN created(CodeGen *gen);
	};


	/**
	 * Class that contains the state of the code generation in many languages. Has basic support for
	 * return values as well.
	 */
	class CodeGen : public Object {
		STORM_CLASS;
	public:
		// Create. Supply which thread we're supposed to run on, if we're generating a member
		// function and the result type.
		STORM_CTOR CodeGen(RunOn thread, Bool member, Value result);

		// Create a new CodeGen which attaches to an already existing listing.
		STORM_CTOR CodeGen(RunOn thread, code::Listing *l);
		STORM_CTOR CodeGen(RunOn thread, code::Listing *l, code::Block block);

		// Create a child.
		STORM_CTOR CodeGen(CodeGen *me, code::Block b);

		// Create a child code gen where another block is the topmost one.
		CodeGen *STORM_FN child(code::Block block);

		// Create a child code gen with a new child block.
		CodeGen *STORM_FN child();

		// Deep copy.
		virtual void STORM_FN deepCopy(CloneEnv *env);

		/**
		 * Create parameters (aware of Storm semantics).
		 */

		// Add a parameter with the specified type.
		code::Var STORM_FN createParam(Value type);

		/**
		 * Create variables (aware of Storm semantics).
		 */

		// Add a variable in the current block.
		VarInfo STORM_FN createVar(Value type);

		// Add a variable in the specified block.
		VarInfo STORM_FN createVar(Value type, code::Block in);


		/**
		 * Return values.
		 */

		// Get the return type.
		Value STORM_FN result() const;

		// Return the value stored in 'value'. Generates an epilog and a ret instruction (i.e. a fnRet instruction).
		void STORM_FN returnValue(code::Var value);

		/**
		 * Data.
		 */

		// Output to here.
		code::Listing *l;

		// Which thread will this code be running on?
		RunOn runOn;

		// Current block.
		code::Block block;

		// toS.
		virtual void STORM_FN toS(StrBuf *to) const;

	private:
		// Return type.
		Value res;
	};


	/**
	 * Resulting value from generated code. It encapsulates a variable (created on demand) that will
	 * store the result from another part of code generation (usually a sub-block).
	 *
	 * This class allows the caller to explicitly ask for the result to be stored in a specific
	 * location, or it can leave that decision up to the callee or to this class.
	 *
	 * TODO: Make toS and deepCopy (no-op).
	 */
	class CodeResult : public Object {
		STORM_CLASS;
	public:
		// No result is needed.
		STORM_CTOR CodeResult();

		// A result of type 't' is needed in 'block'. A variable is created as needed.
		STORM_CTOR CodeResult(Value type, code::Block block);

		// A result of type 't' should be stored in 'var'.
		STORM_CTOR CodeResult(Value type, code::Var var);
		STORM_CTOR CodeResult(Value type, VarInfo var);

		// Get a location to store the variable in. Asserts if the result is not needed.
		code::Var STORM_FN location(CodeGen *s);

		// Get the location of the result even if the result is not needed. Can be used in cases
		// where it is not suitable to avoid creating a result even if it is not needed.
		code::Var STORM_FN safeLocation(CodeGen *s, Value type);

		// Call when the variable was initialized to set up any cleanup required. If you are
		// entirely sure that no cleanup is required for the contained variable, the call to
		// `created` may be omitted.
		void STORM_FN created(CodeGen *s);

		// Suggest a location for the result. Returns true if the suggestion is acceptable,
		// otherwise, it is necessary to use the variable returned by `location`. If the suggestion
		// is used, we assume it is already created (i.e. `created` will be a no-op).
		Bool STORM_FN suggest(CodeGen *s, code::Var v);
		Bool STORM_FN suggest(CodeGen *s, code::Operand v);

		// What type was requested? If `void` then no result is required.
		Value STORM_FN type() const;

		// Does the caller need a result at all?
		Bool STORM_FN needed() const;

		// Create a copy of this result that is usable when multiple pieces of code may produce the
		// result represented by this object. Essentially, this means that cleanup will not be set
		// up until `created` is called on *this* object. Thus, make sure to call `created` here!
		//
		// Call `split` once for each branch that is expected to create a new value, so that each
		// producer gets its own object. Otherwise, issues will arise when they call `created`
		// multiple times.
		CodeResult *STORM_FN split(CodeGen *s);

	private:
		// Variable (invalid if not created yet).
		VarInfo variable;

		// Block `variable` is needed inside.
		code::Block block;

		// Type.
		Value t;
	};

	// Allocate space for 'paramCount' parameters to a FnCall object.
	code::Var STORM_FN createFnCallParams(CodeGen *to, Nat paramCount);

	// Fill in parameter `n` of a FnCall object. Trashes `ptrA` register.
	void STORM_FN setFnParam(CodeGen *to, code::Var params, Nat paramId, code::Operand param);

	// Allocate and fill a `FnCall` object, copying arguments depending on `copy`.
	code::Var STORM_FN createFnCall(CodeGen *to, Array<Value> *formals, Array<code::Operand> *actuals, Bool copy);

	// Allocate an object on the heap. Store it in variable `to`.
	void STORM_FN allocObject(CodeGen *s, Function *ctor, Array<code::Operand> *params, code::Var to);
	code::Var STORM_FN allocObject(CodeGen *s, Function *ctor, Array<code::Operand> *params);

	// Create an object on the heap using the default constructor.
	code::Var STORM_FN allocObject(CodeGen *s, Type *type);

	// Create a thunk for a function usable with function pointers. This thunk is compatible with 'os::CallThunk'
	code::Binary *STORM_FN callThunk(Value result, Array<Value> *params);

	// Make sure all parameters in a parameter list reside in memory, moving any values stored in
	// registers into memory as necessary. Returns either the original parameter list or a modified
	// version if at least one parameter was flushed to memory.
	Array<code::Operand> *STORM_FN spillRegisters(CodeGen *s, Array<code::Operand> *params);

	// Helper for C++ code to conveniently find function objects in Storm. Throws an exception on
	// error. Tip: Use 'valList' to create 'params' conveniently. Uses Scope() as a scope for the lookup.
	// This is not exposed to Storm, since the "named{}" syntax both more efficient and more convenient there.
	Function *findStormFn(NameSet *inside, const wchar *name, Array<Value> *params);
	Function *findStormFn(Value inside, const wchar *name, Array<Value> *params);
	Function *findStormFn(Value inside, const wchar *name);
	Function *findStormFn(Value inside, const wchar *name, Value param0);
	Function *findStormFn(Value inside, const wchar *name, Value param0, Value param1);
	Function *findStormFn(Value inside, const wchar *name, Value param0, Value param1, Value param2);

	// Helper for C++ code to conveniently find a member function. Works like 'findStormFunction',
	// but adds a 'this' pointer as the first parameter automatically.
	Function *findStormMemberFn(Type *inside, const wchar *name, Array<Value> *params);
	Function *findStormMemberFn(Value inside, const wchar *name, Array<Value> *params);
	Function *findStormMemberFn(Value inside, const wchar *name);
	Function *findStormMemberFn(Value inside, const wchar *name, Value param0);
	Function *findStormMemberFn(Value inside, const wchar *name, Value param0, Value param1);
	Function *findStormMemberFn(Value inside, const wchar *name, Value param0, Value param1, Value param2);

}
