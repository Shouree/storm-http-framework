#pragma once
#include "Code/Reg.h"
#include "Code/Output.h"
#include "Code/Operand.h"
#include "Code/CondFlag.h"

namespace code {
	class Listing;
	class TypeDesc;

	namespace arm64 {
		STORM_PKG(core.asm.arm64);

		/**
		 * ARM64 specific registers.
		 *
		 * Since registers are numbered, we don't make constants for all of them.
		 */
		Reg xr(Nat id);
		Reg wr(Nat id);
		Reg ptrr(Nat id);

		Reg dr(Nat id); // Doubles
		Reg sr(Nat id); // Singles
		Reg br(Nat id); // Bytes

		extern const Reg pc;  // Program counter (for addressing).
		extern const Reg sp;  // Stack pointer (always 64-bit).
		extern const Reg pzr; // Zero register (ptr).
		extern const Reg xzr; // Zero register.
		extern const Reg zr;  // Zero register (32-bit).

		// Check if register is integer register.
		Bool isIntReg(Reg r);

		// Check if register is vector register.
		Bool isVectorReg(Reg r);

		// Arm integer register number for register. Returns "out-of-bounds" values for pc, etc.
		Nat intRegNumber(Reg r);

		// Arm register number for reals.
		Nat vectorRegNumber(Reg r);

		// Register name.
		const wchar *nameArm64(Reg r);

		// Condition code for ARM.
		Nat condArm64(CondFlag flag);

		// Registers clobbered by function calls.
		extern const Reg *fnDirtyRegs;
		extern const size_t fnDirtyCount;

		// Get an unused register.
		Reg unusedReg(RegSet *used);

		// Get unused register (as above), but specify desired size.
		Reg unusedReg(RegSet *used, Size size);

		// Get unused register, don't throw if none is available.
		Reg unusedRegUnsafe(RegSet *used);

		// Get unused fp register.
		Reg unusedVectorReg(RegSet *used);
		Reg unusedVectorReg(RegSet *used, Size size);

		// Preserve a register by saving it to a register that is safe through function
		// calls. Returns new location of the operand. It could be in memory.
		// Note: The RegSet is *updated* to match new register allocation.
		Operand preserveReg(Reg reg, RegSet *used, Listing *dest, Block block);

		// As above, but attempts to preserve a register inside a new register. May fail.
		Reg preserveRegInReg(Reg reg, RegSet *used, Listing *dest);

		// Perform a memcpy operation of a fixed size. Uses the two specified registers as
		// temporaries (ARM has load pair and store pair). Copies up to 7 bytes beyond the specified
		// location (i.e., copies a multiple of 8 bytes).
		void inlineMemcpy(Listing *dest, Operand to, Operand from, Reg tmpA, Reg tmpB);

		// Slower version of the above, only able to use one register. Avoid if possible.
		void inlineSlowMemcpy(Listing *dest, Operand to, Operand from, Reg tmpReg);

		// Get a pointer-sized offset into whatever "operand" represents.
		Operand opPtrOffset(Operand op, Nat offset);
		Operand opOffset(Size sz, Operand op, Nat offset);

		// Encode a bitmask. Returns 12-bits N, immr, imms (in that order) if possible. Otherwise,
		// returns 0 (which is not a valid encoding). 'n' is only used if 64-bit bitmask is required.
		Nat encodeBitmask(Word bitmask, bool use64);

		// Check if the word is all ones, taking into account if the value is 64-bit or not.
		Bool allOnes(Word mask, bool use64);

	}
}
