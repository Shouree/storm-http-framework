#pragma once
#include "FnCall.h"
#include "../Transform.h"
#include "../OpTable.h"
#include "../Reg.h"
#include "../Instr.h"
#include "Core/Array.h"

namespace code {
	namespace arm64 {
		STORM_PKG(core.asm.arm64);

		/**
		 * Transform that removes invalid or otherwise non-supported OP-codes, replacing them with
		 * an equivalent sequence of supported OP-codes. In particular, ensures that all arithmetic
		 * operations operate on registers, not on memory.
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

			// Current active block. Mainly used for introducing blocks inside the 'fnCall' transform.
			Block currentBlock;

			// Function parameters.
			Array<ParamInfo> *params;

			// Large operands. Emitted at the end of the function so that we can load them.
			Array<Operand> *large;

			// Label to the start of the large constants section.
			Label lblLarge;

			// Signature for the table of transform functions.
			typedef void (RemoveInvalid::*TransformFn)(Listing *to, Instr *instr, Nat line);

			// Transform table.
			static const OpEntry<TransformFn> transformMap[];

			// Helper function to create a constant for a large integer.
			Operand largeConstant(const Operand &constant);

			// Function calls.
			void fnParamTfm(Listing *to, Instr *instr, Nat line);
			void fnParamRefTfm(Listing *to, Instr *instr, Nat line);
			void fnCallTfm(Listing *to, Instr *instr, Nat line);
			void fnCallRefTfm(Listing *to, Instr *instr, Nat line);

			// Keep track of the current part.
			void prologTfm(Listing *to, Instr *instr, Nat line);
			void beginBlockTfm(Listing *to, Instr *instr, Nat line);
			void endBlockTfm(Listing *to, Instr *instr, Nat line);

			// Fix constraints for specific operations.
			void movTfm(Listing *to, Instr *instr, Nat line);

			// Generic constraint fixing:
			// Constraints for data operations with a 12 bit immediate or a (shifted) register.
			void dataInstr12Tfm(Listing *to, Instr *instr, Nat line);
		};

	}
}
