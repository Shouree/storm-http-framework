#pragma once
#include "Code/Params.h"

namespace code {
	namespace x64 {
		STORM_PKG(core.asm.x64);

		/**
		 * Implements the logic for laying out parameters on the stack and into registers, assuming
		 * Posix ABI.
		 *
		 * On X86-64 parameters are passed according to the following rules:
		 * - The first six integer or pointer arguments are passed in registers
		 *   RDI, RSI, RDX, RCX, R8 and R9.
		 * - The first eight floating-point arguments are passed in registers
		 *   XMM0, XMM1, XMM2, XMM3, XMM4, XMM5, XMM6 and XMM7
		 * - Any parameters that do not fit in registers are passed on the stack.
		 * - Classes containing a trivial copy constructor are decomposed into primitive types
		 *   and passed as separate parameters. However, if the entire class does not fit into
		 *   registers, the entire class is passed on the stack.
		 * - Classes without a trivial copy constructor are passed by pointer. The caller is
		 *   responsible for copying the parameter and destroying it when the function call is
		 *   completed.
		 */
		class PosixParams : public code::Params {
			STORM_CLASS;
		public:
			// Create an empty parameter layout.
			STORM_CTOR PosixParams();

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

	}
}
