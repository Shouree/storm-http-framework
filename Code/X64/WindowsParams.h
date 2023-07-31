#pragma once
#include "Code/Params.h"

namespace code {
	namespace x64 {
		STORM_PKG(core.asm.x64);

		/**
		 * Implements the logic for laying out parameters on the stack and into registers, assuming
		 * Windows ABI.
		 *
		 * The following rules apply on X86-64:
		 * - The first four integer or pointer arguments are passed in registers:
		 *   RCX, RDX, R8, R9
		 * - The first six floating-point arguments are passed in registers:
		 *   XMM0, XMM1, XMM2, XMM3, XMM4, XMM5
		 * - Parameters that do not fit in registers are passed on the stack: passed by
		 *   pointer. Note: documentation states that these should be 16-byte aligned.
		 * - Trivially copiable classes might be decomposed and returned in a single register.
		 */
		class WindowsParams : public code::Params {
			STORM_CLASS;
		public:
			// Create an empty layout.
			STORM_CTOR WindowsParams(Bool member);

			// Get the register containing the parameter.
			Reg STORM_FN registerSrc(Nat n) const;

		protected:
			void STORM_FN resultPrimitive(Primitive p);
			void STORM_FN resultComplex(ComplexDesc *desc);
			void STORM_FN resultSimple(SimpleDesc *desc);

			void STORM_FN addPrimitive(Nat id, Primitive p);
			void STORM_FN addComplex(Nat id, ComplexDesc *desc);
			void STORM_FN addSimple(Nat id, SimpleDesc *desc);

		private:
			// Is this a member function?
			Bool member;
		};

	}
}
