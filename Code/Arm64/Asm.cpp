#include "stdafx.h"
#include "Asm.h"
#include "../Listing.h"
#include "../Exception.h"

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
		// 0x?50..0x?5F <-> q0..q15
		// 0x?60..0x?6F <-> q16..q31

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
				return 0x04B;
			else if (arm == 32)
				return ptrStack;
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
				return 32; // sp
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
					return 31; // xzr
				else if (storm == 0x4C)
					return 33; // pc
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
			return Reg(0x850 + id);
		}

		Reg dr(Nat id) {
			return Reg(0x450 + id);
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

		Nat vectorRegNumber(Reg r) {
			Nat z = Nat(r) & 0xFF;
			assert(z >= 0x50 && z <= 0x6F, L"Invalid ARM vector register.");
			return z - 0x50;
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
				ARM_REG_SPECIAL(31, "zr");
				if (number == 33)
					return S("pc");
			} else if (isVectorReg(r)) {
				return S("<vector,todo>");
			}

			return null;
		}

		Nat condArm64(CondFlag flag) {
			switch (flag) {
			case ifAlways:
				return 0xE;
			case ifNever:
				return 0xF;
			case ifOverflow:
				return 0x6;
			case ifNoOverflow:
				return 0x7;
			case ifEqual:
				return 0x0;
			case ifNotEqual:
				return 0x1;

				// Unsigned compare:
			case ifBelow:
				return 0x3;
			case ifBelowEqual:
				return 0x9;
			case ifAboveEqual:
				return 0x2;
			case ifAbove:
				return 0x8;

				// Singned comparision.
			case ifLess:
				return 0xB;
			case ifLessEqual:
				return 0xD;
			case ifGreaterEqual:
				return 0xA;
			case ifGreater:
				return 0xC;

				// Float comparision.
			case ifFBelow:
				return 0x3;
			case ifFBelowEqual:
				return 0x9;
			case ifFAboveEqual:
				return 0xA;
			case ifFAbove:
				return 0xC;
			}

			return 0xE;
		}

		Reg unusedReg(RegSet *used) {
			Reg r = unusedRegUnsafe(used);
			if (r == noReg)
				throw new (used) InvalidValue(S("We should not run out of registers on ARM64."));
			return r;
		}

		Reg unusedReg(RegSet *used, Size size) {
			return asSize(unusedReg(used), size);
		}

		Reg unusedRegUnsafe(RegSet *used) {
			static const Reg candidates[] = {
				ptrr(0), ptrr(1), ptrr(2), ptrr(3), ptrr(4), ptrr(5), ptrr(6), ptrr(7), ptrr(8),
				ptrr(9), ptrr(10), ptrr(11), ptrr(12), ptrr(13), ptrr(14), ptrr(15), ptrr(16), ptrr(17),
				ptrr(19), ptrr(20), ptrr(21), ptrr(22), ptrr(23), ptrr(24), ptrr(25), ptrr(26), ptrr(27),
				ptrr(28),
			};
			for (Nat i = 0; i < ARRAY_COUNT(candidates); i++)
				if (!used->has(candidates[i]))
					return candidates[i];
			return noReg;
		}

		static const Reg dirtyRegs[] = {
			ptrr(0), ptrr(1), ptrr(2), ptrr(3), ptrr(4), ptrr(5), ptrr(6), ptrr(7), ptrr(8),
			ptrr(9), ptrr(10), ptrr(11), ptrr(12), ptrr(13), ptrr(14), ptrr(15), ptrr(16), ptrr(17),
			dr(0), dr(1), dr(2), dr(3), dr(4), dr(5), dr(6), dr(7),
		};
		const Reg *fnDirtyRegs = dirtyRegs;
		const size_t fnDirtyCount = ARRAY_COUNT(dirtyRegs);


		Reg preserveRegInReg(Reg reg, RegSet *used, Listing *dest) {
			Reg targetReg = noReg;
			if (isIntReg(reg)) {
				for (Nat i = 19; i < 29; i++) {
					if (used->has(ptrr(i)))
						continue;

					targetReg = ptrr(i);
					break;
				}
			} else {
				for (Nat i = 8; i < 16; i++) {
					if (used->has(dr(i)))
						continue;

					targetReg = dr(i);
					break;
				}
			}

			used->remove(reg);

			if (targetReg != noReg) {
				targetReg = asSize(targetReg, size(reg));
				used->put(targetReg);
				*dest << mov(targetReg, reg);
				return targetReg;
			}

			return noReg;
		}

		Operand preserveReg(Reg reg, RegSet *used, Listing *dest, Block block) {
			Reg targetReg = preserveRegInReg(reg, used, dest);
			if (targetReg != noReg)
				return targetReg;

			// Store on the stack.
			Var to = dest->createVar(block, size(reg));
			*dest << mov(to, reg);
			return to;
		}

		// Get a pointer-sized offset into whatever "operand" represents.
		Operand opPtrOffset(Operand op, Nat offset) {
			return opOffset(Size::sPtr, op, offset);
		}

		Operand opOffset(Size sz, Operand op, Nat offset) {
			switch (op.type()) {
			case opRelative:
				return xRel(sz, op.reg(), op.offset() + Offset(offset));
			case opVariable:
				return xRel(sz, op.var(), op.offset() + Offset(offset));
			default:
				assert(false, L"Unsupported operand passed to 'opOffset'!");
			}
		}

		void inlineMemcpy(Listing *dest, Operand to, Operand from, Reg tmpA, Reg tmpB) {
			Nat size = from.size().size64();
			if (size <= 8) {
				*dest << mov(asSize(tmpA, from.size()), from);
				*dest << mov(to, asSize(tmpA, from.size()));
				return;
			}

			// Make them pointer-sized.
			tmpA = asSize(tmpA, Size::sPtr);
			tmpB = asSize(tmpB, Size::sPtr);

			Nat offset = 0;
			while (offset + 16 <= size) {
				// The backend will make this into a double-load.
				*dest << mov(tmpA, opPtrOffset(from, offset));
				*dest << mov(tmpB, opPtrOffset(from, offset + 8));

				// The backend will make this into a double-store.
				*dest << mov(opPtrOffset(to, offset), tmpA);
				*dest << mov(opPtrOffset(to, offset + 8), tmpB);

				offset += 16;
			}

			// Copy remaining 8 bytes (up to machine alignment, typically OK).
			if (offset < size) {
				*dest << mov(tmpA, opPtrOffset(from, offset));
				*dest << mov(opPtrOffset(to, offset), tmpA);
			}
		}

	}
}
