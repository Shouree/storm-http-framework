#include "stdafx.h"
#include "Asm.h"

namespace code {
	namespace arm64 {

		// We map registers as follows:
		// ptrStack (1) <-> sp
		// ptrFrame (2) <-> x29
		// ptrA (3) <-> x0
		// ptrB (4) <-> x1
		// ptrC (5) <-> x2
		// 0x?30..0x?3F <-> x3..x18
		// 0x?40..0x?4F <-> x19..x28,x30,xzr,pc
		// 0x?50..0x?50 <-> q0..q15
		// 0x?60..0x?60 <-> q16..q31

		// Arm integer register to storm register.
		Nat armIntToStorm(Nat arm) {
			if (arm <= 2)
				return 0x003 + arm;
			else if (arm <= 18)
				return 0x030 + arm - 3;
			else if (arm <= 28)
				return 0x040 + arm - 19;
			else if (arm == 29)
				return ptrFrame;
			else if (arm == 30)
				return 0x04A;
			else if (arm == 31)
				return ptrStack;
			else if (arm == 32)
				return 0x04B;
			else if (arm == 33)
				return 0x04C;
			else
				return noReg;
		}

		// Storm reg number to Arm int register.
		Nat stormToArmInt(Reg stormReg) {
			Nat storm = stormReg & 0xFF;
			Nat type = storm >> 4;

			if (storm == 0x01) {
				// SP.
				return 31;
			} else if (storm == 0x2) {
				// Reg. 29 is frame ptr.
				return 29;
			} else if (type == 0x0) {
				return storm - 0x3;
			} else if (type == 0x3) {
				return (storm & 0xF) + 3;
			} else if (type == 0x4) {
				if (storm < 0x4A)
					return storm - 0x40 + 19;
				else if (storm == 0x4A)
					return 30;
				else if (storm == 0x4B)
					return 32;
				else if (storm == 0x4C)
					return 33;
			}

			return -1;
		}

		Reg xr(Nat id) {
			return Reg(armIntToStorm(id) | 0x800);
		}

		Reg wr(Nat id) {
			return Reg(armIntToStorm(id) | 0x400);
		}

		Reg ptrr(Nat id) {
			return Reg(armIntToStorm(id) | 0x000);
		}

		Reg qr(Nat id) {
			return noReg;
		}

		Reg dr(Nat id) {
			return noReg;
		}

		const Reg pc = Reg(0x04C);
		const Reg sp = ptrStack;
		const Reg pzr = Reg(0x04B);
		const Reg xzr = Reg(0x84B);
		const Reg zr = Reg(0x44B);

		Bool isIntReg(Reg r) {
			Nat cat = r & 0x0F0;
			return cat == 0x000 || cat == 0x030 || cat == 0x40;
		}

		Bool isVectorReg(Reg r) {
			Nat cat = r & 0x0F0;
			return cat = 0x050 || cat == 0x060;
		}

		Nat intRegNumber(Reg r) {
			return stormToArmInt(r);
		}

#define ARM_REG_SPECIAL(NR, NAME)				\
		if (number == NR) {						\
			if (size == 0) {					\
				return S("px" NAME);			\
			} else if (size == 4) {				\
				return S("w" NAME);				\
			} else if (size == 8) {				\
				return S("x" NAME);				\
			} else if (size == 1) {				\
				return S("b" NAME);				\
			}									\
	}

#define ARM_REG_CASE(NR)						\
		ARM_REG_SPECIAL(NR, #NR)

		const wchar *nameArm64(Reg r) {
			Nat number = stormToArmInt(r);
			Nat size = r >> 8;

			if (isIntReg(r)) {
				ARM_REG_CASE(0);
				ARM_REG_CASE(1);
				ARM_REG_CASE(2);
				ARM_REG_CASE(3);
				ARM_REG_CASE(4);
				ARM_REG_CASE(5);
				ARM_REG_CASE(6);
				ARM_REG_CASE(7);
				ARM_REG_CASE(8);
				ARM_REG_CASE(9);
				ARM_REG_CASE(10);
				ARM_REG_CASE(11);
				ARM_REG_CASE(12);
				ARM_REG_CASE(13);
				ARM_REG_CASE(14);
				ARM_REG_CASE(15);
				ARM_REG_CASE(16);
				ARM_REG_CASE(17);
				ARM_REG_CASE(18);
				ARM_REG_CASE(19);
				ARM_REG_CASE(20);
				ARM_REG_CASE(21);
				ARM_REG_CASE(22);
				ARM_REG_CASE(23);
				ARM_REG_CASE(24);
				ARM_REG_CASE(25);
				ARM_REG_CASE(26);
				ARM_REG_CASE(27);
				ARM_REG_CASE(28);
				ARM_REG_CASE(29);
				ARM_REG_CASE(30);
				ARM_REG_SPECIAL(32, "zr");
				if (number == 33)
					return S("pc");
			} else if (isVectorReg(r)) {
				return S("<vector,todo>");
			}

			return null;
		}

	}
}
