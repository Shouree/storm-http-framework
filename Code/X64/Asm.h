#pragma once
#include "../Reg.h"
#include "../Output.h"
#include "../Operand.h"

namespace code {
	class Listing;
	class TypeDesc;

	namespace x64 {
		STORM_PKG(core.asm.x64);

		/**
		 * X86-64 specific registers.
		 *
		 * TODO: Expose to Storm somehow.
		 */
		extern const Reg ptrD;
		extern const Reg ptrSi;
		extern const Reg ptrDi;
		extern const Reg ptr8;
		extern const Reg ptr9;
		extern const Reg ptr10;
		extern const Reg ptr11;
		extern const Reg ptr12;
		extern const Reg ptr13;
		extern const Reg ptr14;
		extern const Reg ptr15;
		extern const Reg dl;
		extern const Reg sil;
		extern const Reg dil;
		extern const Reg r8l;
		extern const Reg r9l;
		extern const Reg r10l;
		extern const Reg r11l;
		extern const Reg r12l;
		extern const Reg r13l;
		extern const Reg r14l;
		extern const Reg r15l;
		extern const Reg edx;
		extern const Reg esi;
		extern const Reg edi;
		extern const Reg rdx;
		extern const Reg rsi;
		extern const Reg rdi;
		extern const Reg e8;
		extern const Reg e9;
		extern const Reg e10;
		extern const Reg e11;
		extern const Reg e12;
		extern const Reg e13;
		extern const Reg e14;
		extern const Reg e15;
		extern const Reg r8;
		extern const Reg r9;
		extern const Reg r10;
		extern const Reg r11;
		extern const Reg r12;
		extern const Reg r13;
		extern const Reg r14;
		extern const Reg r15;

		/**
		 * XMM registers required for the calling convention and FP operations. Called 'emm' for 4
		 * bytes wide and 'xmm' for 8 bytes wide. These are only usable in 'mov' instructions. It is
		 * not possible to move data from a regular register to an xmm register directly.
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
		const wchar *nameX64(Reg r);

		// Register ID.
		nat registerId(Reg r);

		// Is this a xmm register?
		bool fpRegister(Reg r);
		bool fpRegister(const Operand &op);
		nat fpRegisterId(Reg r);

		// Code for conditional operations.
		byte condOp(CondFlag c);

		// Does 'value' fit in a single byte?
		bool singleByte(Word value);
		bool singleByte(Nat value);

		// Does 'value' fit in a 32-bit word?
		bool singleInt(Word value);

		// Find a unused register given a set of used registers.
		Reg unusedReg(RegSet *in);

		// As above, but returns 'noReg' instead of throwing if no registers are available.
		Reg unusedRegUnsafe(RegSet *in);

		// Find an unused fp register given a set of used registers.
		Reg unusedFpReg(RegSet *in);

		// Description of an op-code.
		struct OpCode {
			// Any prefix for this op-code? Prefixes are always emitted before any REX
			// prefix. Prefixes are always non-zero, so 0x00 means no prefix.
			byte prefix;

			// The actual op-code. Maximum 2 bytes. If only one byte is required, the first bytes
			// are 0. (the byte 0x00 is the ADD instruction which is 1 byte long. Thus 0x00 0x00
			// uniquely identifies ADD).
			byte op1;
			byte op2;
		};

		// Create OpCode objects.
		inline OpCode opCode(byte op) {
			OpCode r = { 0x00, 0x00, op };
			return r;
		}

		inline OpCode opCode(byte op1, byte op2) {
			OpCode r = { 0x00, op1, op2 };
			return r;
		}

		inline OpCode prefixOpCode(byte prefix, byte op) {
			OpCode r = { prefix, 0x00, op };
			return r;
		}

		inline OpCode prefixOpCode(byte prefix, byte op1, byte op2) {
			OpCode r = { prefix, op1, op2 };
			return r;
		}

		// Output an opcode.
		void put(Output *to, OpCode op);

		// Output an opcode with a REX prefix.
		void put(Output *to, byte rex, OpCode op);

		// Flags to the modRm function, telling it how to behave.
		enum RmFlags {
			// No special flags.
			rmNone = 0x0,
			// This is a wide instruction.
			rmWide = 0x1,
			// This is a byte instruction. We will use the registers DIL and SIL instead of AH, BH, etc.
			// Can be used with 'rmWide', but that is usually useless.
			rmByte = 0x2,
		};

		BITMASK_OPERATORS(RmFlags);

		// Wide operand?
		inline RmFlags wide(const Operand &op) {
			if (op.size() == Size::sWord || op.size() == Size::sPtr)
				return rmWide;
			else
				return rmNone;
		}

		// Output an instruction with a ModRm modifier afterwards. Emits a REX prefix if necessary.
		void modRm(Output *to, OpCode op, RmFlags flags, const Operand &dest, const Operand &src);
		void modRm(Output *to, OpCode op, RmFlags flags, nat mode, const Operand &dest);

		// Describes a single instruction that takes an immediate value or a register, along with a ModRm operand.
		struct ImmRegInstr {
			// OP-code and mode when followed by an 8-bit immediate value. If 'modeImm' == 0xFF, imm8 is not used.
			OpCode opImm8;
			byte modeImm8;

			// OP-code and mode when followed by a 32-bit immediate value.
			OpCode opImm32;
			byte modeImm32;

			// OP-code when using a register as a source.
			OpCode opSrcReg;

			// OP-code when using a register as a destination.
			OpCode opDestReg;
		};

		// Describes a single instruction with 8 bit operands.
		struct ImmRegInstr8 {
			OpCode opImm;
			byte modeImm;

			OpCode opSrcReg;
			OpCode opDestReg;
		};

		// Emit an instruction represented by ImmRegInstr. Supports the following addressing modes:
		// register, *
		// *, register
		// *, immediate
		void immRegInstr(Output *to, const ImmRegInstr &op, const Operand &dest, const Operand &src);
		void immRegInstr(Output *to, const ImmRegInstr8 &op, const Operand &dest, const Operand &src);
		void immRegInstr(Output *to, const ImmRegInstr8 &op8, const ImmRegInstr &op, const Operand &dest, const Operand &src);

		// Get a pointer-sized offset into whatever "operand" represents.
		Operand opPtrOffset(Operand op, Nat offset);
		Operand opOffset(Size sz, Operand op, Nat offset);
	}
}
