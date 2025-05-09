#pragma once
#include "Code/Params.h"

namespace code {
	namespace arm64 {
		STORM_PKG(core.asm.arm64);

		/**
		 * Implements the logic for laying out parameters on the stack and into registers.
		 *
		 * On ARM64 parameters are passed according to the following rules:
		 * - The first eight integer or pointer parameters are passed in registers x0..x7
		 * - The first eight floating-point arguments are passed in registers d0..d7
		 * - Any parameters that do not fit in registers are passed on the stack.
		 * - Composite types that are not "trivial" are passed in memory (replaced by a pointer in the parameter list).
		 * - Composite types are passed on the stack if they are larger than 16 bytes.
		 * - Composite types of size up to 16 bytes might be split into two registers.
		 * - Note: Arguments copied to the stack will have to be aligned to 8 bytes, even
		 *   if their natural alignment is smaller.
		 * - If a structure contains at most four elements of a single fp-type, it is passed
		 *   as separate values in the fp registers (called a HFA in the ABI).
		 *
		 * In summary, some important differences from the X86-64 calling convention are:
		 * - Each parameter gets allocated to *one* set of registers. It is never split between
		 *   integer registers and fp registers.
		 * - Return values are handled as "regular" parameters.
		 * - No special cases with regards to the "this" parameter.
		 * - Register numbers are "aligned" when large structs are passed.
		 * - FP registers are *only* used for uniform float parameters. I.e., not if a struct
		 *   contains e.g. 2 floats and 1 double.
		 */
		class Params : public code::Params {
			STORM_CLASS;
		public:
			// Create an empty parameter layout.
			STORM_CTOR Params();

			// Get the register containing the parameter.
			Reg STORM_FN registerSrc(Nat n) const;

		protected:
			void STORM_FN resultPrimitive(Primitive p);
			void STORM_FN resultComplex(ComplexDesc *desc);
			void STORM_FN resultSimple(SimpleDesc *desc);

			void STORM_FN addPrimitive(Nat id, Primitive p);
			void STORM_FN addComplex(Nat id, ComplexDesc *desc);
			void STORM_FN addSimple(Nat id, SimpleDesc *desc);
		};

		// Create a 'params' object from a list of TypeDesc objects. Note: The result never affects
		// the parameter layout on Arm64, so no parameter with the result id will ever appear in the
		// returned object.
		Params *STORM_FN layoutParams(TypeDesc *result, Array<TypeDesc *> *types);

	}
}
