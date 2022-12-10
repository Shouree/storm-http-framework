#pragma once
#include "../Transform.h"
#include "../OpTable.h"
#include "../UsedRegs.h"

namespace code {
	namespace x86 {
		STORM_PKG(core.asm.x86);

		/**
		 * Transform which replaces invalid instructions with a sequence of valid ones.
		 */
		class RemoveInvalid : public Transform {
			STORM_CLASS;
		public:
			STORM_CTOR RemoveInvalid();

			// Start transform.
			virtual void STORM_FN before(Listing *dest, Listing *src);

			// Transform one instruction.
			virtual void STORM_FN during(Listing *dest, Listing *src, Nat id);

		private:
			// Remember used registers.
			Array<RegSet *> *used;

			// Find an unused register at 'line'.
			Reg unusedReg(Nat line);

			// Struct describing parameters for a future fnCall instruction.
			class Param {
				STORM_VALUE;
			public:
				Param(Operand src, TypeDesc *type, Bool ref);

				// Source operand.
				Operand src;

				// Source type.
				TypeDesc *type;

				// Reference parameter?
				Bool ref;
			};

			// Function params seen so far.
			Array<Param> *params;

			// Signature for the table of transform functions.
			typedef void (RemoveInvalid::*TransformFn)(Listing *dest, Instr *instr, Nat line);

			// Transform table.
			static const OpEntry<TransformFn> transformMap[];

			// Load floating-point register.
			Reg loadFpRegister(Listing *dest, const Operand &op, Nat line);
			Operand loadFpRegisterOrMemory(Listing *dest, const Operand &op, Nat line);

			// Transform functions.
			void immRegTfm(Listing *dest, Instr *instr, Nat line);
			void beginBlockTfm(Listing *dest, Instr *instr, Nat line);
			void leaTfm(Listing *dest, Instr *instr, Nat line);
			void mulTfm(Listing *dest, Instr *instr, Nat line);
			void idivTfm(Listing *dest, Instr *instr, Nat line);
			void udivTfm(Listing *dest, Instr *instr, Nat line);
			void imodTfm(Listing *dest, Instr *instr, Nat line);
			void umodTfm(Listing *dest, Instr *instr, Nat line);
			void setCondTfm(Listing *dest, Instr *instr, Nat line);
			void shlTfm(Listing *dest, Instr *instr, Nat line);
			void shrTfm(Listing *dest, Instr *instr, Nat line);
			void sarTfm(Listing *dest, Instr *instr, Nat line);
			void icastTfm(Listing *dest, Instr *instr, Nat line);
			void ucastTfm(Listing *dest, Instr *instr, Nat line);
			void fnegTfm(Listing *dest, Instr *instr, Nat line);
			void fcastiTfm(Listing *dest, Instr *instr, Nat line);
			void fcastuTfm(Listing *dest, Instr *instr, Nat line);
			void icastfTfm(Listing *dest, Instr *instr, Nat line);
			void ucastfTfm(Listing *dest, Instr *instr, Nat line);

			// Generic transform for floating point operations.
			void fpInstrTfm(Listing *dest, Instr *instr, Nat line);

			// Perform a function call.
			void fnCall(Listing *dest, TypeInstr *instr, Array<Param> *params);

			// Function calls.
			void fnParamTfm(Listing *dest, Instr *instr, Nat line);
			void fnParamRefTfm(Listing *dest, Instr *instr, Nat line);
			void fnCallTfm(Listing *dest, Instr *instr, Nat line);
			void fnCallRefTfm(Listing *dest, Instr *instr, Nat line);
		};

	}
}
