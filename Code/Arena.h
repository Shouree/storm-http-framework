#pragma once
#include "Core/TObject.h"
#include "Core/EnginePtr.h"
#include "Output.h"
#include "Operand.h"

namespace code {
	STORM_PKG(core.asm);

	class Listing;
	class Binary;
	class RegSet;
	class TypeDesc;

	/**
	 * An arena represents a collection of compiled code and external references for some architecture.
	 *
	 * Abstract class, there is one instantiation for each supported platform.
	 */
	class Arena : public ObjectOn<Compiler> {
		STORM_CLASS;
	public:
		// Create an arena.
		Arena();

		// Create external references.
		Ref external(const wchar *name, const void *ptr) const;
		RefSource *externalSource(const wchar *name, const void *ptr) const;

		/**
		 * Transform and translate code into machine code.
		 */

		// Transform the code in preparation for this backend's code generation. This is
		// backend-specific. 'owner' is the binary object that will be called to handle exceptions.
		virtual Listing *STORM_FN transform(Listing *src, Binary *owner) const;

		// Translate a previously transformed listing into machine code for this arena.
		virtual void STORM_FN output(Listing *src, Output *to) const;

		/**
		 * Create output objects for this backend.
		 */

		// Create an offset-computing output.
		virtual LabelOutput *STORM_FN labelOutput() const;

		// Create a code-generating output. 'lblOffsets' and 'size' are obtained through 'labelOutput'.
		virtual CodeOutput *STORM_FN codeOutput(Binary *owner, Array<Nat> *offsets, Nat size, Nat refs) const;
		CodeOutput *STORM_FN codeOutput(Binary *owner, LabelOutput *src) const;

		// Remove all registers not preserved during a function call on this platform. This
		// implementation removes ptrA, ptrB and ptrC, but other Arena implementations may want to
		// remove others as well.
		virtual void STORM_FN removeFnRegs(RegSet *from) const;

		/**
		 * Other backend-specific things.
		 */

		// Create a function that calls another function (optionally with a pointer sized parameter)
		// to figure out which function to actually call. Useful when implementing lazy compilation.
		//
		// Calls 'fn' with 'param' (always pointer-sized or empty) to compute the
		// actual function to call. The actual function (as well as the 'function' implemented by
		// the redirect) takes params as defined by 'params' and returns 'result'.
		//
		// These redirect objects are *not* platform independent!
		virtual Listing *STORM_FN redirect(Bool member, TypeDesc *result, Array<TypeDesc *> *params, Ref fn, Operand param);

		// Create a function that calls another (pre-determined) function and appends an 'EnginePtr'
		// object as the first parameter to the other function. Calling member functions in this
		// manner is not supported.
		virtual Listing *STORM_FN engineRedirect(TypeDesc *result, Array<TypeDesc *> *params, Ref fn, Operand engine);


		/**
		 * Get the location of the first parameter for a function call. Assumes that a member function is called.
		 *
		 * The location is acquired in two steps: first, an implementation asks the ID of the
		 * parameter location by calling the 'firstParamId(TypeDesc *)' function. This returns one
		 * out of several possible integers describing the parameter location. The number of
		 * possible values can be acquired by calling 'firstParamId(null)'.
		 *
		 * The ID can then be passed to 'firstParamLoc' to get an Operand describing the location.
		 *
		 * This scheme is used so that classes like VTableCalls can detect when two functions with
		 * different return values have the same vtable stub. This allows it to re-use the stubs.
		 */

		// Get the ID of the location of the first param.
		virtual Nat STORM_FN firstParamId(MAYBE(TypeDesc *) desc);

		// Access the location of the first parameter in a function size. The returned Operand is
		// always pointer-sized.
		virtual Operand STORM_FN firstParamLoc(Nat id);
	};

	// Create an arena for this platform.
	Arena *STORM_FN arena(EnginePtr e);
}
