#pragma once
#include "FnCall.h"
#include "../Transform.h"
#include "../OpTable.h"
#include "../Reg.h"
#include "../Instr.h"
#include "Core/Array.h"

namespace code {
	namespace x64 {
		STORM_PKG(core.asm.x64);

		/**
		 * Transform that removes invalid or otherwise non-supported OP-codes, replacing them with
		 * an equivalent sequence of supported OP-codes. For example:
		 * - memory-memory operands
		 *   (Note: references and object references are stored in memory, and therefore count as a memory operand!)
		 * - immediate values that require 64 bits (need to be stored in memory as well)
		 */
		class RemoveInvalid : public Transform {
			STORM_CLASS;
		public:
			STORM_CTOR RemoveInvalid();

			// Start transform.
			virtual void STORM_FN before(Listing *dest, Listing *src);

			// Transform a single instruction.
			virtual void STORM_FN during(Listing *dest, Listing *src, Nat id);

			// Finalize transform.
			virtual void STORM_FN after(Listing *dest, Listing *src);

		private:
			// Used registers for each line.
			Array<RegSet *> *used;

			// Remember all 'large' constants that need to be stored at the end of the code section.
			Array<Operand> *large;

			// Label to the start of the large constants section.
			Label lblLarge;

			// Extract any large numbers from an instruction.
			Instr *extractNumbers(Instr *i);

			// Remove any references to complex parameters.
			Instr *extractComplex(Listing *dest, Instr *i, Nat line);

			// Load floating-point register.
			Reg loadFpRegister(Listing *dest, const Operand &op, Nat line);
			Operand loadFpRegisterOrMemory(Listing *dest, const Operand &op, Nat line);

			// Parameters for an upcoming 'fnCall' instruction.
			Array<ParamInfo> *params;

			// Current active block. We need that for introducing blocks inside the 'fnCall' transform.
			Block currentBlock;

			// Signature for the table of transform functions.
			typedef void (RemoveInvalid::*TransformFn)(Listing *dest, Instr *instr, Nat line);

			// Transform table.
			static const OpEntry<TransformFn> transformMap[];

			// Generic transform function for instructions using a generic two-op immReg form.
			void immRegTfm(Listing *dest, Instr *instr, Nat line);

			// Generic transform for instructions requiring their dest operand to be a register.
			void destRegWTfm(Listing *dest, Instr *instr, Nat line);
			void destRegRwTfm(Listing *dest, Instr *instr, Nat line);

			// Generic transform for floating point operations.
			void fpInstrTfm(Listing *dest, Instr *instr, Nat line);

			// Function calls.
			void fnParamTfm(Listing *dest, Instr *instr, Nat line);
			void fnParamRefTfm(Listing *dest, Instr *instr, Nat line);
			void fnCallTfm(Listing *dest, Instr *instr, Nat line);
			void fnCallRefTfm(Listing *dest, Instr *instr, Nat line);

			// Keep track of the current part.
			void prologTfm(Listing *dest, Instr *instr, Nat line);
			void beginBlockTfm(Listing *dest, Instr *instr, Nat line);
			void endBlockTfm(Listing *dest, Instr *instr, Nat line);

			// Other transforms.
			void shlTfm(Listing *dest, Instr *instr, Nat line);
			void shrTfm(Listing *dest, Instr *instr, Nat line);
			void sarTfm(Listing *dest, Instr *instr, Nat line);
			void idivTfm(Listing *dest, Instr *instr, Nat line);
			void imodTfm(Listing *dest, Instr *instr, Nat line);
			void udivTfm(Listing *dest, Instr *instr, Nat line);
			void umodTfm(Listing *dest, Instr *instr, Nat line);
			void fnegTfm(Listing *dest, Instr *instr, Nat line);
			void fcastiTfm(Listing *dest, Instr *instr, Nat line);
			void fcastuTfm(Listing *dest, Instr *instr, Nat line);
			void icastfTfm(Listing *dest, Instr *instr, Nat line);
			void ucastfTfm(Listing *dest, Instr *instr, Nat line);
		};

	}
}
