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

			// Helper function to create a constant for a large integer.
			Operand largeConstant(const Operand &constant);

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

			// Label to integer division by zero code, if necessary.
			Label lblDivZero;

			// Get the division by zero label if needed.
			Label divZeroLabel(Listing *l);

			// Signature for the table of transform functions.
			typedef void (RemoveInvalid::*TransformFn)(Listing *to, Instr *instr, Nat line);

			// Transform table.
			static const OpEntry<TransformFn> transformMap[];

			// Generic function for wrapping an instruction between loads and stores.
			void removeMemoryRefs(Listing *to, Instr *instr, Nat line);

			// Generic function for loading data into a register. Handles cases that a plain mov
			// instruction does not handle without transformation.
			void loadRegister(Listing *to, Reg reg, const Operand &load);

			// Load value into fp register.
			Reg loadFpRegister(Listing *to, const Operand &load, Nat line);

			// Limit operands with large constants to the desired length. Otherwise replaces it with
			// code to load the constant into a suitable register.
			Operand limitImm(Listing *to, const Operand &op, Nat line, Nat immBits, Bool immSigned);

			// Same as 'limitImm', but only affects the source of the instruction.
			Instr *limitImmSrc(Listing *to, Instr *instr, Nat line, Nat immBits, Bool immSigned);

			// Generate code that ensures that 'op' is either a constant of a particular size, or a
			// register.
			Operand regOrImm(Listing *to, const Operand &op, Nat line, Nat immBits, Bool immSigned);

			// Modify instructions for byte-sized operands.
			Instr *modifyByte(Listing *to, Instr *instr, Nat line);
			Operand modifyByte(Listing *to, const Operand &op, Nat line, Bool opSigned);

			// Function calls.
			void fnParamTfm(Listing *to, Instr *instr, Nat line);
			void fnParamRefTfm(Listing *to, Instr *instr, Nat line);
			void fnCallTfm(Listing *to, Instr *instr, Nat line);
			void fnCallRefTfm(Listing *to, Instr *instr, Nat line);

			// Keep track of the current part.
			void prologTfm(Listing *to, Instr *instr, Nat line);
			void epilogTfm(Listing *to, Instr *instr, Nat line);
			void beginBlockTfm(Listing *to, Instr *instr, Nat line);
			void endBlockTfm(Listing *to, Instr *instr, Nat line);

			// Fix constraints for specific operations.
			void movTfm(Listing *to, Instr *instr, Nat line);
			void leaTfm(Listing *to, Instr *instr, Nat line);
			void swapTfm(Listing *to, Instr *instr, Nat line);
			void cmpTfm(Listing *to, Instr *instr, Nat line);
			void setCondTfm(Listing *to, Instr *instr, Nat line);
			void icastTfm(Listing *to, Instr *instr, Nat line);
			void ucastTfm(Listing *to, Instr *instr, Nat line);
			void fcastTfm(Listing *to, Instr *instr, Nat line);
			void fcastiTfm(Listing *to, Instr *instr, Nat line);
			void fcastuTfm(Listing *to, Instr *instr, Nat line);
			void icastfTfm(Listing *to, Instr *instr, Nat line);
			void ucastfTfm(Listing *to, Instr *instr, Nat line);

			// Transform mod into div + multiply as needed. No mod operation on ARM!
			void idivTfm(Listing *to, Instr *instr, Nat line);
			void udivTfm(Listing *to, Instr *instr, Nat line);
			void imodTfm(Listing *to, Instr *instr, Nat line);
			void umodTfm(Listing *to, Instr *instr, Nat line);

			// Generic constraint fixing:
			// Constraints for data operations with a 12 bit immediate or a (shifted) register.
			void dataInstr12Tfm(Listing *to, Instr *instr, Nat line);
			// Constraints for data operations with a bitmask-type immediate or a (shifted) register.
			void bitmaskInstrTfm(Listing *to, Instr *instr, Nat line);
			// Constraints for 4-reg data operations where all operands need to be in memory.
			void dataInstr4RegTfm(Listing *to, Instr *instr, Nat line);
			// For shift operations, where 'src' is either an immediate (that needs to be small), or a register.
			void shiftInstrTfm(Listing *to, Instr *instr, Nat line);
			// For floating-point arithmetic operations.
			void fpInstrTfm(Listing *to, Instr *instr, Nat line);

		};

	}
}
