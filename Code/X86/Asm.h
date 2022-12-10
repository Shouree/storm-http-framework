#pragma once
#include "../Reg.h"
#include "../Output.h"
#include "../Operand.h"
#include "../TypeDesc.h"
#include "Core/EnginePtr.h"

namespace code {
	class Listing;

	namespace x86 {
		STORM_PKG(core.asm.x86);

		/**
		 * X86-specific registers.
		 */
		extern const Reg ptrD;
		extern const Reg ptrSi;
		extern const Reg ptrDi;
		extern const Reg dl;
		extern const Reg sil;
		extern const Reg dil;
		extern const Reg edx;
		extern const Reg esi;
		extern const Reg edi;
		extern const Reg rdx;
		extern const Reg rsi;
		extern const Reg rdi;

		/**
		 * XMM registers for fp operations (using sse). Called 'emm' for 4 bytes and 'xmm' for 8
		 * bytes wide.
		 */
		extern const Reg emm0;
		extern const Reg emm1;
		extern const Reg emm2;
		extern const Reg emm3;
		extern const Reg emm4;
		extern const Reg emm5;
		extern const Reg emm6;
		extern const Reg emm7;
		extern const Reg xmm0;
		extern const Reg xmm1;
		extern const Reg xmm2;
		extern const Reg xmm3;
		extern const Reg xmm4;
		extern const Reg xmm5;
		extern const Reg xmm6;
		extern const Reg xmm7;


		// Convert to names.
		const wchar *nameX86(Reg r);

		// Find unused registers to work with. Returns noReg if none. Takes 64-bit registers into account.
		Reg STORM_FN unusedReg(RegSet *in);

		// Find an unused fp register given a set of used registers.
		Reg STORM_FN unusedFpReg(RegSet *in);

		// Add 64-bit registers if needed.
		void STORM_FN add64(RegSet *to);

		// Get the low or high 32-bit register. If 'r' is a 32-bit register, high32 returns noReg.
		Reg STORM_FN low32(Reg r);
		Reg STORM_FN high32(Reg r);
		Operand STORM_FN low32(Operand o);
		Operand STORM_FN high32(Operand o);

		// All registers.
		extern const Reg *allRegs;
		extern const size_t allCount;

		// Registers clobbered by function calls.
		extern const Reg *fnDirtyRegs;
		extern const size_t fnDirtyCount;

		// Find the register id of a register.
		nat registerId(Reg r);

		// Is this a xmm register?
		bool fpRegister(Reg r);
		bool fpRegister(const Operand &op);
		nat fpRegisterId(Reg r);

		// Get the 'op-code' for conditional operators.
		byte condOp(CondFlag c);

		// See if this value can be expressed in a single byte without loss. Assumes sign extension.
		bool singleByte(Word value);

		// Generates a SIB-byte that express the address: baseReg + scaledReg*scale.
		// NOTE: scale must be either 1, 2, 4 or 8.
		// NOTE: baseReg == ebp does _not_ work.
		// NOTE: scaledReg == esp does _not_ work.
		// When scaledReg == -1 (0xFF) no scaling is used. (scale must be 1)
		byte sibValue(byte baseReg, byte scaledReg, byte scale);
		inline byte sibValue(byte baseReg, byte scaledReg) { return sibValue(baseReg, scaledReg, 1); }
		inline byte sibValue(byte baseReg) { return sibValue(baseReg, -1); }

		// Assemble a modRmValue from its parts.
		byte modRmValue(byte mode, byte src, byte dest);

		// Compute 'modRm' for subOp, dest. Write to 'to'.
		void modRm(Output *to, byte subOp, const Operand &dest);

		// Describes a single instruction that takes either an immediate value, or a register.
		struct ImmRegInstr {
			// The op-code and mode when followed by an 8-bit immediate value. If 'modeImm8' == 0xFF
			// the imm8 is not used. If 'modeImm8' == 0x0F (prefix), the opImm8 is treated as a prefixed op-code.
			byte opImm8, modeImm8;
			// The op-code and mode when followed by a 32-bit immediate value.
			byte opImm32, modeImm32;
			// The op-code when using a register as source.
			byte opSrcReg;
			// The op-code when using a register as a destination.
			byte opDestReg;
		};

		// Describes a single instruction that takes either an immediate value, or a register. 8-bit operands.
		struct ImmRegInstr8 {
			// Op-code and mode when followed by an 8-bit immediate value.
			byte opImm, modeImm;
			// Op-code when using a register as source.
			byte opSrcReg;
			// Op-code when using a register as destination.
			byte opDestReg;
		};

		// Generates code for an instruction described by the ImmRegInstr. Do not support both src and
		// dest to be "strange" addressing modes. The following modes are supported:
		// register, *
		// *, register
		// *, constant
		// Use "immRegTransform" to expand op-codes to a move before the op-code, so that we can handle all combinations.
		void immRegInstr(Output *to, const ImmRegInstr &op, const Operand &dest, const Operand &src);
		void immRegInstr(Output *to, const ImmRegInstr8 &op, const Operand &dest, const Operand &src);
		void immRegInstr(Output *to, const ImmRegInstr8 &op8, const ImmRegInstr &op, const Operand &dest, const Operand &src);

		/**
		 * Preserve and restore registers.
		 */

		// Preserve a single register.
		Reg STORM_FN preserve(Reg r, RegSet *used, Listing *dest);

		// Restore a single register.
		void STORM_FN restore(Reg r, Reg saved, Listing *dest);

		/**
		 * Preserve a set of registers. Assumed to be stored on the stack somewhere.
		 */
		class Preserve {
		public:
			Preserve(const Reg *preserve, size_t preserveCount, RegSet *used, Listing *dest);

			void restore();
		private:
			Listing *dest;
			Array<Nat> *srcReg;
			Array<Nat> *destReg;
		};


		// Shall a result of the type 'r' be returned on the stack? Ie. do we need an additional
		// parameter for the result?
		bool resultParam(TypeDesc *desc);

		// Encode/decode block-id and activation-id into a format we store in a single word on the stack.
		Nat encodeFnState(Nat block, Nat activation);
		void decodeFnState(Nat original, Nat &block, Nat &activation);

	}
}
