#pragma once
#include "Code/Reg.h"
#include "Code/Output.h"
#include "Code/Operand.h"

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

		Reg qr(Nat id);
		Reg dr(Nat id);

		extern const Reg pc;  // Program counter (for addressing).
		extern const Reg sp;  // Stack pointer (always 64-bit).
		extern const Reg pzr; // Zero register (ptr).
		extern const Reg xzr; // Zero register.
		extern const Reg zr;  // Zero register (32-bit).

		// Check if register is integer register.
		Bool isIntReg(Reg r);

		// Check if register is vector register.
		Bool isVectorReg(Reg r);

		// Arm integer register number for register.
		Nat intRegNumber(Reg r);

		// Register name.
		const wchar *nameArm64(Reg r);

	}
}
