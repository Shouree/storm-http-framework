#include "stdafx.h"
#include "AsmOut.h"
#include "Asm.h"
#include "../OpTable.h"
#include "../Exception.h"

namespace code {
	namespace arm64 {

		// Get register number where 31=sp.
		static Nat intRegSP(Reg reg) {
			Nat r = intRegNumber(reg);
			if (r < 31)
				return r;
			if (r == 32)
				return 31;

			throw new (runtime::someEngine()) InternalError(S("Can not use this register with this op-code."));
		}

		// Get register number where 31=zr.
		static Nat intRegZR(Reg reg) {
			Nat r = intRegNumber(reg);
			if (r < 32)
				return r;

			throw new (runtime::someEngine()) InternalError(S("Can not use this register with this op-code."));
		}

		// Get fp register number.
		static Nat fpReg(Reg reg) {
			return vectorRegNumber(reg);
		}

		// Good reference for instruction encoding:
		// https://developer.arm.com/documentation/ddi0596/2021-12/Index-by-Encoding?lang=en

		// Check if value fits in 6-bit signed.
		static Bool isImm6S(Long value) {
			return value >= -0x20 && value <= 0x1F;
		}
		static void checkImm6S(RootObject *e, Long value) {
			if (!isImm6S(value))
				throw new (e) InvalidValue(TO_S(e, S("Too large signed 6-bit immediate value: ") << value));
		}

		// Check if value fits in 6-bit unsigned.
		static Bool isImm6U(Word value) {
			return value <= 0x3F;
		}
		static void checkImm6U(RootObject *e, Long value) {
			if (!isImm6U(value))
				throw new (e) InvalidValue(TO_S(e, S("Too large unsigned 6-bit immediate value: ") << value));
		}

		// Check if value fits in 7-bit signed.
		static Bool isImm7S(Long value) {
			return value >= -0x40 && value <= 0x3F;
		}
		static void checkImm7S(RootObject *e, Long value) {
			if (!isImm7S(value))
				throw new (e) InvalidValue(TO_S(e, S("Too large signed 7-bit immediate value: ") << value));
		}

		// Check if value fits in 7-bit unsigned.
		static Bool isImm7U(Word value) {
			return value <= 0x7F;
		}
		static void checkImm7U(RootObject *e, Word value) {
			if (!isImm7U(value))
				throw new (e) InvalidValue(TO_S(e, S("Too large unsigned 7-bit immediate value: ") << value));
		}

		// Check if value fits in 9-bit signed.
		static Bool isImm9S(Long value) {
			return value >= -0x100 && value <= 0xFF;
		}
		static void checkImm9S(RootObject *e, Long value) {
			if (!isImm9S(value))
				throw new (e) InvalidValue(TO_S(e, S("Too large signed 9-bit immediate value: ") << value));
		}

		// Check if value fits in 9-bit unsigned.
		static Bool isImm9U(Word value) {
			return value <= 0x1FF;
		}
		static void checkImm9U(RootObject *e, Word value) {
			if (!isImm9U(value))
				throw new (e) InvalidValue(TO_S(e, S("Too large unsigned 9-bit immediate value: ") << value));
		}

		// Check if value fits in 12-bit signed.
		static Bool isImm12S(Long value) {
			return value >= -0x800 && value <= 0x7FF;
		}
		static void checkImm12S(RootObject *e, Long value) {
			if (!isImm12S(value))
				throw new (e) InvalidValue(TO_S(e, S("Too large signed 12-bit immediate value: ") << value));
		}

		// Check if value fits in 12-bit unsigned.
		static Bool isImm12U(Word value) {
			return value <= 0xFFF;
		}
		static void checkImm12U(RootObject *e, Word value) {
			if (!isImm12U(value))
				throw new (e) InvalidValue(TO_S(e, S("Too large unsigned 12-bit immediate value: ") << value));
		}

		// Check if value fits in 19-bit signed.
		static Bool isImm19S(Long value) {
			return value >= -0x40000 && value <= 0x3FFFF;
		}
		static void checkImm19S(RootObject *e, Long value) {
			if (!isImm19S(value))
				throw new (e) InvalidValue(TO_S(e, S("Too large signed 19-bit immediate value: ") << value));
		}

		// Check if value fits in 19-bit unsigned.
		static Bool isImm19U(Word value) {
			return value <= 0x7FFFF;
		}
		static void checkImm19U(RootObject *e, Long value) {
			if (!isImm19U(value))
				throw new (e) InvalidValue(TO_S(e, S("Too large unsigned 19-bit immediate value: ") << value));
		}

		// Check if value fits in 26-bit signed.
		static Bool isImm26S(Long value) {
			return value >= -0x02000000 && value <= 0x03FFFFFF;
		}
		static void checkImm26S(RootObject *e, Long value) {
			if (!isImm26S(value))
				throw new (e) InvalidValue(TO_S(e, S("Too large signed 26-bit immediate value: ") << value));
		}

		// Put data instructions. 2 registers, 12-bit unsigned immediate.
		static inline void putData2(Output *to, Nat op, Nat rDest, Nat rSrc, Word imm) {
			checkImm12U(to, imm);
			Nat instr = (op << 22) | rDest | (rSrc << 5) | ((imm & 0xFFF) << 10);
			to->putInt(instr);
		}

		// Put data instructions. 3 registers, and a 6-bit unsigned immediate. Some instructions allow 'rb' to be shifted.
		static inline void putData3(Output *to, Nat op, Nat rDest, Nat ra, Nat rb, Word imm) {
			checkImm6U(to, imm);
			Nat instr = (op << 21) | rDest | (rb << 16) | (ra << 5) | ((imm & 0x3F) << 10);
			to->putInt(instr);
		}

		// Put 3-input data instructions. 4 registers, no immediate (modifier is labelled oO in the
		// docs, part of OP-code for some instructions it seems).
		static inline void putData4a(Output *to, Nat op, Bool modifier, Nat rDest, Nat ra, Nat rb, Nat rc) {
			Nat instr = (op << 21) | rDest | (rc << 10) | (rb << 16) | (ra << 5) | (Nat(modifier) << 15);
			to->putInt(instr);
		}

		// Same as putData4a, except that the 'modifier' is in another place. Labelled o1 in the docs.
		static inline void putData4b(Output *to, Nat op, Bool modifier, Nat rDest, Nat ra, Nat rb, Nat rc) {
			Nat instr = (op << 21) | rDest | (rc << 16) | (rb << 11) | (ra << 5) | (Nat(modifier) << 10);
			to->putInt(instr);
		}

		// Put a bitmask operation (those that have N, immr, imms in them). The bitmask will be
		// encoded, and size will be added if needed.
		static inline void putBitmask(Output *to, Nat op, Nat rSrc, Nat rDest, Bool is64, Word bitmask) {
			Nat encBitmask = encodeBitmask(bitmask, is64);
			if (encBitmask == 0) {
				StrBuf *msg = new (to) StrBuf();
				*msg << S("It is not possible to encode the value ") << hex(bitmask)
					 << S(" as a bitmask. It should have been removed by an earlier pass.");
				throw new (to) InvalidValue(msg->toS());
			}
			Nat instr = (op << 23) | rDest | (rSrc << 5) | (encBitmask << 10);
			if (is64)
				instr |= Nat(1) << 31;
			to->putInt(instr);
		}

		// Put instructions for loads and stores: 3 registers and a 7-bit signed immediate.
		static inline void putLoadStoreS(Output *to, Nat op, Nat base, Nat r1, Nat r2, Long imm) {
			checkImm7S(to, imm);
			Nat instr = (op << 22) | r1 | (base << 5) | (r2 << 10) | ((imm & 0x7F) << 15);
			to->putInt(instr);
		}

		// Put instructions for loads and stores: 3 registers and a 7-bit unsigned immediate.
		static inline void putLoadStoreU(Output *to, Nat op, Nat base, Nat r1, Nat r2, Word imm) {
			checkImm7U(to, imm);
			Nat instr = (op << 22) | r1 | (base << 5) | (r2 << 10) | ((imm & 0x7F) << 15);
			to->putInt(instr);
		}

		// Put a "mid-sized" load/store (for negative offsets, mainly): 2 registers and 9-bit immediate.
		static inline void putLoadStoreMidS(Output *to, Nat op, Nat base, Nat r1, Long imm) {
			checkImm9S(to, imm);
			Nat instr = (op << 21) | r1 | (base << 5) | ((0x1FF & imm) << 12);
			to->putInt(instr);
		}

		// Put a "large" load/store (for bytes, mainly): 2 registers and 12-bit immediate.
		static inline void putLoadStoreLargeS(Output *to, Nat op, Nat base, Nat r1, Long imm) {
			checkImm12S(to, imm);
			Nat instr = (op << 22) | r1 | (base << 5) | ((0xFFF & imm) << 10);
			to->putInt(instr);
		}

		// Put a "large" load/store (for bytes, mainly): 2 registers and 12-bit immediate.
		static inline void putLoadStoreLargeU(Output *to, Nat op, Nat base, Nat r1, Word imm) {
			checkImm12U(to, imm);
			Nat instr = (op << 22) | r1 | (base << 5) | ((0xFFF & imm) << 10);
			to->putInt(instr);
		}

		// Put a load/store with 19-bit immediate offset from PC.
		static inline void putLoadStoreImm(Output *to, Nat op, Nat reg, Long imm) {
			checkImm19S(to, imm);
			Nat instr = (op << 24) | reg | ((0x7FFFF & imm) << 5);
			to->putInt(instr);
		}

		// Load a "long" constant into a register. Uses the table of references to store the data.
		static inline void loadLongConst(Output *to, Nat reg, Ref value) {
			// Emit "ldr" with literal, make the literal refer to location in the table after the code block.
			putLoadStoreImm(to, 0x58, reg, 0);
			to->markGc(GcCodeRef::relativeHereImm19, 4, value);
		}
		static inline void loadLongConst(Output *to, Nat reg, RootObject *obj) {
			// Emit "ldr" with literal, make the literal refer to location in the table after the code block.
			putLoadStoreImm(to, 0x58, reg, 0);
			to->markGc(GcCodeRef::relativeHereImm19, 4, (Word)obj);
		}

		void nopOut(Output *to, Instr *instr) {
			// According to ARM manual.
			to->putInt(0xD503201F);
		}

		void prologOut(Output *to, Instr *instr) {
			Offset stackSize = instr->src().offset();
			Int scaled = stackSize.v64() / 8;
			if (isImm7S(scaled)) {
				// Small enough: we can do the modifications in the store operation:

				// - stp x29, x30, [sp, -stackSize]!
				putLoadStoreS(to, 0x2A6, 31, 29, 30, -scaled);

				// New offset for stack:
				to->setFrameOffset(stackSize);

				// The fact that registers have been preserved is done after the if-stmt as it is the same for all cases.

			} else if (scaled <= 0xFFFF) {
				// Too large. Load value into register (inter-procedure clobbered x16 is good for this).

				// - mov x16, #<scaled>
				to->putInt(0xD2800000 | ((scaled & 0xFFFF) << 5) | 16);
				// - sub sp, sp, (x16 << 3)
				putData3(to, 0x659, 31, 31, 16, 3 << 3 | 3); // flags mean shift 3 steps, treat as unsigned 64-bit
				// CFA offset is now different:
				to->setFrameOffset(stackSize);
				// - stp x29, x30, [sp]
				putLoadStoreU(to, 0x2A4, 31, 29, 30, 0);

			} else {
				// Note: In reality, the stack size is likely a bit smaller since we are limited by
				// pointer offsets, since they are limited to 14 bytes (=16 KiB).
				throw new (to) InvalidValue(S("Too large stack size for Arm64!"));
			}

			// We have now saved the stack pointer and the return pointer:
			to->markSaved(xr(29), -stackSize);
			to->markSaved(xr(30), -(stackSize - Size::sPtr));

			// - mov x29, sp   # create stack frame
			putData2(to, 0x244, 29, 31, 0);
			// Now: use x29 as the frame register instead!
			to->setFrameRegister(xr(29));
		}

		void epilogOut(Output *to, Instr *instr) {
			Offset stackSize = instr->src().offset();
			Int scaled = stackSize.v64() / 8;
			if (isImm7S(scaled)) {
				// We emit:
				// - ldp x29, x30, [sp], stackSize
				putLoadStoreS(to, 0x2A3, 31, 29, 30, scaled);
			} else if (scaled <= 0xFFFF) {
				// The inverse of the prolog is:

				// - ldp x29, x30, [sp]
				putLoadStoreU(to, 0x2A5, 31, 29, 30, 0);
				// - mov x16, #<scaled>
				to->putInt(0xD2800000 | ((scaled & 0xFFFF) << 5) | 16);
				// - add sp, sp, (x16 << 3)
				putData3(to, 0x459, 31, 31, 16, 3 << 3 | 3); // flags mean shift 3 steps, treat as unsigned 64-bit

			} else {
				throw new (to) InvalidValue(S("Too large stack size for Arm64!"));
			}

			// Note: No DWARF metadata since this could be an early return.
		}

		bool loadOut(Output *to, Instr *instr, MAYBE(Instr *) next) {
			Reg baseReg = instr->src().reg();
			Int offset = instr->src().offset().v64();
			Int opSize = instr->dest().size().size64();
			Reg dest1 = instr->dest().reg();
			Reg dest2 = noReg;

			Bool intReg = isIntReg(dest1);

			// Bytes are special:
			if (opSize == 1) {
				if (offset < 0)
					putLoadStoreMidS(to, intReg ? 0x1C2 : 0x1E2, intRegSP(baseReg), intRegZR(dest1), offset);
				else
					putLoadStoreLargeU(to, intReg ? 0x0E5 : 0x0F5, intRegSP(baseReg), intRegZR(dest1), offset);
				return false;
			}

			// Look at "next" to see if we can merge it with this instruction.
			if (next && next->dest().type() == opRegister && next->src().type() == opRelative) {
				if (same(next->src().reg(), baseReg) && Int(next->dest().size().size64()) == opSize) {
					// Note: It is undefined to load the same register multiple times. Also: it
					// might break semantics when turning:
					// - ldr x0, [x0]
					// - ldr x0, [x0+8]
					// into
					// - ldp x0, x0, [x0]
					if (!same(next->dest().reg(), dest1) && isIntReg(next->dest().reg()) == intReg) {
						// Look at the offsets, if they are next to each other, we can merge them.
						Int off = next->src().offset().v64();
						if (off == offset + opSize && isImm7S(offset / opSize)) {
							dest2 = next->dest().reg();
						} else if (off == offset - opSize && isImm7S(off / opSize)) {
							// Put the second one first.
							dest2 = dest1;
							dest1 = next->dest().reg();
							offset = off;
						}
					}
				}
			}

			if (offset % opSize)
				throw new (to) InvalidValue(S("Memory access on Arm must be aligned!"));
			offset /= opSize;

			// Interestingly enough, ldp takes a signed offset but ldr takes an unsigned offset.

			if (dest2 != noReg) {
				if (intReg)
					putLoadStoreS(to, opSize == 4 ? 0x0A5 : 0x2A5, intRegSP(baseReg), intRegZR(dest1), intRegZR(dest2), offset);
				else
					putLoadStoreS(to, opSize == 4 ? 0x0B5 : 0x1B5, intRegSP(baseReg), fpReg(dest1), fpReg(dest2), offset);
			} else if (offset < 0) {
				// Here: We need to use 'ldur' instead. That one is unscaled.
				if (intReg)
					putLoadStoreMidS(to, opSize == 4 ? 0x5C2 : 0x7C2, intRegSP(baseReg), intRegZR(dest1), offset * opSize);
				else
					putLoadStoreMidS(to, opSize == 4 ? 0x5E2 : 0x7E2, intRegSP(baseReg), intRegZR(dest1), offset * opSize);
			} else {
				if (intReg)
					putLoadStoreLargeU(to, opSize == 4 ? 0x2E5 : 0x3E5, intRegSP(baseReg), intRegZR(dest1), offset);
				else
					putLoadStoreLargeU(to, opSize == 4 ? 0x2F5 : 0x3F5, intRegSP(baseReg), fpReg(dest1), offset);
			}

			return dest2 != noReg;
		}

		bool storeOut(Output *to, Instr *instr, MAYBE(Instr *) next) {
			Reg baseReg = instr->dest().reg();
			Int offset = instr->dest().offset().v64();
			Int opSize = instr->src().size().size64();
			Reg src1 = instr->src().reg();
			Reg src2 = noReg;

			Bool intReg = isIntReg(src1);

			// Bytes are special:
			if (opSize == 1) {
				if (offset < 0)
					putLoadStoreMidS(to, intReg ? 0x1C0 : 0x1E0, intRegSP(baseReg), intRegZR(src1), offset);
				else
					putLoadStoreLargeU(to, intReg ? 0x0E4 : 0x0F4, intRegSP(baseReg), intRegZR(src1), offset);
				return false;
			}

			// Look at "next" to see if we can merge it with this instruction.
			if (next && next->src().type() == opRegister && next->dest().type() == opRelative) {
				if (same(next->dest().reg(), baseReg) && Int(next->src().size().size64()) == opSize) {
					// Note: Contrary to the load instruction, it is well-defined to store the same
					// register multiple times.
					if (isIntReg(next->src().reg()) == intReg) {
						// Look at the offsets, if they are next to each other, we can merge them.
						Int off = next->dest().offset().v64();
						if (off == offset + opSize && isImm7S(offset / opSize)) {
							src2 = next->src().reg();
						} else if (off == offset - opSize && isImm7S(off / opSize)) {
							// Put the second one first.
							src2 = src1;
							src1 = next->src().reg();
							offset = off;
						}
					}
				}
			}

			if (offset % opSize)
				throw new (to) InvalidValue(S("Memory access on Arm must be aligned!"));
			offset /= opSize;

			// Interestingly enough, LDP takes a signed offset while LDR takes an unsigned offset...

			if (src2 != noReg) {
				if (intReg)
					putLoadStoreS(to, opSize == 4 ? 0x0A4 : 0x2A4, intRegSP(baseReg), intRegZR(src1), intRegZR(src2), offset);
				else
					putLoadStoreS(to, opSize == 4 ? 0x0B4 : 0x1B4, intRegSP(baseReg), fpReg(src1), fpReg(src2), offset);
			} else if (offset < 0) {
				// Here: we need to use 'ldur' instead.
				if (intReg)
					putLoadStoreMidS(to, opSize == 4 ? 0x5C0 : 0x7C0, intRegSP(baseReg), intRegZR(src1), offset * opSize);
				else
					putLoadStoreMidS(to, opSize == 4 ? 0x5E0 : 0x7E0, intRegSP(baseReg), intRegZR(src1), offset * opSize);
			} else {
				if (intReg)
					putLoadStoreLargeU(to, opSize == 4 ? 0x2E4 : 0x3E4, intRegSP(baseReg), intRegZR(src1), offset);
				else
					putLoadStoreLargeU(to, opSize == 4 ? 0x2F4 : 0x3F4, intRegSP(baseReg), fpReg(src1), offset);
			}

			return src2 != noReg;
		}

		void regRegMove(Output *to, Reg dest, Reg src) {
			Bool intSrc = isIntReg(src);
			Bool intDst = isIntReg(dest);
			if (intSrc && intDst) {
				if (src == ptrStack || dest == ptrStack)
					putData2(to, 0x244, intRegSP(dest), intRegSP(src), 0);
				else if (size(src).size64() > 4)
					putData3(to, 0x550, intRegZR(dest), 31, intRegZR(src), 0);
				else
					putData3(to, 0x150, intRegZR(dest), 31, intRegZR(src), 0);
			} else if (!intSrc && !intDst) {
				if (size(src).size64() > 4)
					to->putInt(0x1E602000 | (fpReg(src) << 5) | fpReg(dest));
				else
					to->putInt(0x1E202000 | (fpReg(src) << 5) | fpReg(dest));
			} else if (intSrc) {
				if (size(src).size64() > 4)
					to->putInt(0x9E670000 | (intRegZR(src) << 5) | fpReg(dest));
				else
					to->putInt(0x1E270000 | (intRegZR(src) << 5) | fpReg(dest));
			} else if (intDst) {
				if (size(src).size64() > 4)
					to->putInt(0x9E660000 | (fpReg(src) << 5) | intRegZR(dest));
				else
					to->putInt(0x1E260000 | (fpReg(src) << 5) | intRegZR(dest));
			}
		}

		// Special version called directly when more than one mov was found. Returns "true" if we
		// could merge the two passed to us. We know that "next" is a mov op if it is non-null.
		bool movOut(Output *to, Instr *instr, MAYBE(Instr *) next) {
			switch (instr->dest().type()) {
			case opRegister:
				// Fall through to next switch statement.
				break;
			case opRelative:
				if (instr->src().type() != opRegister)
					throw new (to) InvalidValue(TO_S(to, S("Invalid source for store operation on ARM: ") << instr->src()));
				return storeOut(to, instr, next);
			default:
				throw new (to) InvalidValue(TO_S(to, S("Invalid destination for move operation: ") << instr));
			}

			// dest is a register!
			Reg destReg = instr->dest().reg();
			Operand src = instr->src();

			switch (src.type()) {
			case opRegister:
				regRegMove(to, destReg, src.reg());
				return false;
			case opRelative:
				return loadOut(to, instr, next);
			case opReference:
				// Must be a pointer: Also, dest must be register.
				loadLongConst(to, intRegZR(destReg), src.ref());
				return false;
			case opObjReference:
				// Must be a pointer, and dest must be a register.
				loadLongConst(to, intRegZR(destReg), src.object());
				return false;
			case opConstant:
				if (src.constant() > 0xFFFF)
					throw new (to) InvalidValue(TO_S(to, S("Too large immediate to load: ") << src));
				// Note: No difference between nat and word version.
				to->putInt(0xD2800000 | ((0xFFFF & src.constant()) << 5) | intRegZR(destReg));
				return false;
			case opLabel:
			case opRelativeLbl: {
				Int offset = to->offset(src.label()) + src.offset().v64() - to->tell();
				Bool large = size(destReg).size64() > 4;
				if (isIntReg(destReg)) {
					putLoadStoreImm(to, large ? 0x58 : 0x18, intRegZR(destReg), offset / 4);
				} else {
					putLoadStoreImm(to, large ? 0x5C : 0x1C, fpReg(destReg), offset / 4);
				}
				return false;
			}
			default:
				throw new (to) InvalidValue(TO_S(to, S("Invalid source for move operation: ") << instr));
			}
		}

		void movOut(Output *to, Instr *instr) {
			movOut(to, instr, null);
		}

		void leaOut(Output *to, Instr *instr) {
			Operand destOp = instr->dest();
			if (destOp.type() != opRegister)
				throw new (to) InvalidValue(S("Destination of lea should have been transformed to a register."));

			Nat dest = intRegZR(destOp.reg());

			Operand src = instr->src();
			switch (src.type()) {
			case opRelative:
				// Note: These are not sign-extended, so we need to be careful about the sign.
				if (src.offset().v64() > 0) {
					// add:
					putData2(to, 0x244, dest, intRegZR(src.reg()), src.offset().v64());
				} else {
					// sub:
					putData2(to, 0x344, dest, intRegZR(src.reg()), -src.offset().v64());
				}
				break;
			case opReference:
				// This means to load the refSource instead of loading from the pointer.
				loadLongConst(to, dest, src.refSource());
				break;
			default:
				throw new (to) InvalidValue(TO_S(to, S("Unsupported source operand for lea: ") << src));
			}
		}

		void callOut(Output *to, Instr *instr) {
			// Note: We need to use x17 for temporary values. This is assumed by the code in Gc/CodeArm64.cpp.

			Nat offset;
			Operand target = instr->src();
			switch (target.type()) {
			case opReference:
				// Load addr. into x17.
				putLoadStoreImm(to, 0x58, 17, 0);
				// blr x17
				to->putInt(0xD63F0220);
				// Mark it accordingly.
				to->markGc(GcCodeRef::jump, 8, target.ref());
				break;
			case opRegister:
				to->putInt(0xD63F0000 | (intRegZR(target.reg()) << 5));
				break;
			case opRelative:
				// Split into two op-codes: a load and a register call.
				offset = target.offset().v64() / 8;
				putLoadStoreLargeU(to, 0x3E5, intRegZR(target.reg()), 17, offset);
				// blr x17
				to->putInt(0xD63F0220);
				break;
			default:
				assert(false, L"Unsupported call target!");
				break;
			}
		}

		void retOut(Output *to, Instr *) {
			to->putInt(0xD65F03C0);
		}

		void jmpCondOut(Output *to, CondFlag cond, const Operand &target) {
			Int offset;

			switch (target.type()) {
			case opLabel:
				offset = Int(to->offset(target.label())) - Int(to->tell());
				offset /= 4;
				checkImm19S(to, offset);
				to->putInt(0x54000000 | ((Nat(offset) & 0x7FFFF) << 5) | condArm64(cond));
				break;
			default:
				assert(false, L"Unsupported target for conditional branches.");
				break;
			}
		}

		void jmpOut(Output *to, Instr *instr) {
			CondFlag cond = instr->src().condFlag();
			Operand target = instr->dest();
			Int offset;

			if (cond == ifNever)
				return;

			if (cond != ifAlways) {
				// Conditional jumps are special, handle them separately.
				jmpCondOut(to, cond, target);
				return;
			}

			// Note: We need to use x17 for temporary values for long jumps. This is assumed by the
			// code in Gc/CodeArm64.cpp.

			switch (target.type()) {
			case opReference:
				// Load addr. into x17.
				putLoadStoreImm(to, 0x58, 17, 0);
				// br x17
				to->putInt(0xD61F0220);
				// Mark it accordingly.
				to->markGc(GcCodeRef::jump, 8, target.ref());
				break;
			case opRegister:
				to->putInt(0xD61F0000 | (intRegZR(target.reg()) << 5));
				break;
			case opRelative:
				// Split into two op-codes: a load and a register jump.
				offset = target.offset().v64() / 8;
				putLoadStoreLargeU(to, 0x3E5, intRegZR(target.reg()), 17, offset);
				// br x17
				to->putInt(0xD61F0220);
				break;
			case opLabel:
				offset = Int(to->offset(target.label())) - Int(to->tell());
				offset /= 4;
				checkImm26S(to, offset);
				to->putInt(0x14000000 | (Nat(offset) & 0x03FFFFFF));
				break;
			default:
				assert(false, L"Unsupported jump target!");
				break;
			}
		}

		// Generic output of data instructions that use 12-bit immediates or registers. Assumes that
		// the high bit of the op-code is the size bit.
		static void data12Out(Output *to, Instr *instr, Nat opImm, Nat opReg) {
			assert(instr->dest().type() == opRegister,
				L"Destinations for data operations should have been transformed into registers.");
			Nat dest = intRegSP(instr->dest().reg());

			if (instr->src().size().size64() >= 8) {
				opImm |= 0x200;
				opReg |= 0x400;
			}

			switch (instr->src().type()) {
			case opRegister:
				putData3(to, opReg, dest, dest, intRegSP(instr->src().reg()), 0);
				break;
			case opConstant:
				putData2(to, opImm, dest, dest, instr->src().constant());
				break;
			default:
				assert(false, L"Unsupported source for data operation.");
				break;
			}
		}

		void addOut(Output *to, Instr *instr) {
			data12Out(to, instr, 0x044, 0x058);
		}

		void subOut(Output *to, Instr *instr) {
			data12Out(to, instr, 0x144, 0x258);
		}

		void cmpOut(Output *to, Instr *instr) {
			assert(instr->dest().type() == opRegister,
				L"Src and dest for cmp should have been transformed into registers.");
			Nat dest = intRegZR(instr->dest().reg());

			Nat opImm = 0x1C4;
			Nat opReg = 0x358;

			if (instr->src().size().size64() >= 8) {
				opImm |= 0x200;
				opReg |= 0x400;
			}

			switch (instr->src().type()) {
			case opRegister:
				putData3(to, opReg, 31, dest, intRegSP(instr->src().reg()), 0);
				break;
			case opConstant:
				putData2(to, opImm, 31, dest, instr->src().constant());
				break;
			default:
				assert(false, L"Unsupported source for data operation.");
				break;
			}
		}

		void setCondOut(Output *to, Instr *instr) {
			Nat dest = intRegZR(instr->dest().reg());
			CondFlag cond = instr->src().condFlag();
			// Note: There is no "if never" condition, so we need a special case for that. Since we
			// need to invert the condition, we just special-case both always and never.
			if (cond == ifAlways) {
				to->putInt(0xD2800020 | dest);
			} else if (cond == ifNever) {
				to->putInt(0xD2800000 | dest);
			} else {
				to->putInt(0x1A9F07E0 | dest | (condArm64(inverse(instr->src().condFlag())) << 12));
			}
		}

		void mulOut(Output *to, Instr *instr) {
			// Everything has to be in registers here.
			Operand src = instr->src();
			Operand dest = instr->dest();

			Nat op = 0x0D8;
			if (src.size().size64() >= 8)
				op |= 0x400;

			Nat destReg = intRegZR(dest.reg());
			putData4a(to, op, false, destReg, destReg, intRegZR(src.reg()), 31);
		}

		static void divOut(Output *to, Instr *instr, Bool sign) {
			Operand src = instr->src();
			Operand dest = instr->dest();

			Nat op = 0x0D6;
			if (src.size().size64() >= 8)
				op |= 0x400;
			Nat destReg = intRegZR(dest.reg());
			putData4b(to, op, sign, destReg, destReg, 0x1, intRegZR(src.reg()));
		}

		void idivOut(Output *to, Instr *instr) {
			divOut(to, instr, true);
		}

		void udivOut(Output *to, Instr *instr) {
			divOut(to, instr, false);
		}

		static void clampSize(Output *to, Reg reg, Nat size) {
			if (size == 1) {
				// This is AND with an encoded bitmask.
				to->putInt(0x92401C00 | (intRegSP(reg) << 5) | intRegZR(reg));
			} else if (size == 4) {
				// This is AND with an encoded bitmask.
				to->putInt(0x92407C00 | (intRegSP(reg) << 5) | intRegZR(reg));
			} else {
				// No need to clamp larger values.
			}
		}

		void icastOut(Output *to, Instr *instr) {
			Operand src = instr->src();
			Operand dst = instr->dest();
			Nat srcSize = src.size().size64();
			Nat dstSize = dst.size().size64();

			if (!isIntReg(dst.reg()))
				throw new (to) InvalidValue(S("Can not sign extend floating point registers."));

			// Source is either register or memory reference.
			if (src.type() == opRelative) {
				// Use a suitable load instruction.
				Int offset = instr->src().offset().v64();

				if (srcSize == 1) {
					Nat op = 0x0E6;
					putLoadStoreLargeU(to, op, intRegSP(src.reg()), intRegZR(dst.reg()), offset);
				} else if (srcSize == 4) {
					Nat op = 0x2E6;
					putLoadStoreLargeU(to, op, intRegSP(src.reg()), intRegZR(dst.reg()), offset / 4);
				} else {
					// This is a regular load.
					Nat op = 0x3E5;
					putLoadStoreLargeU(to, op, intRegSP(src.reg()), intRegZR(dst.reg()), offset / 8);
				}

				// Maybe clamp to smaller size.
				if (srcSize > dstSize)
					clampSize(to, dst.reg(), dstSize);
			} else if (src.type() == opRegister) {
				// Sign extend to 64 bits:
				if (srcSize == 1) {
					// Insn: sxtb
					Nat op = 0x93401C00 | (intRegZR(src.reg()) << 5) | intRegZR(dst.reg());
					to->putInt(op);
				} else if (srcSize == 4) {
					Nat op = 0x93407C00 | (intRegZR(src.reg()) << 5) | intRegZR(dst.reg());
					to->putInt(op);
				}

				clampSize(to, dst.reg(), dstSize);
			}
		}

		void ucastOut(Output *to, Instr *instr) {
			Operand src = instr->src();
			Operand dst = instr->dest();
			Nat srcSize = src.size().size64();
			Nat dstSize = dst.size().size64();

			Bool intDst = isIntReg(dst.reg());

			// Source is either register or memory reference.
			if (src.type() == opRelative) {
				// Use a suitable load instruction.
				Int offset = instr->src().offset().v64();

				if (srcSize == 1) {
					Nat op = intDst ? 0x0E5 : 0x0F5;
					putLoadStoreLargeU(to, op, intRegSP(src.reg()), intRegZR(dst.reg()), offset);
				} else if (srcSize == 4) {
					Nat op = intDst ? 0x2E5 : 0x2F5;
					putLoadStoreLargeU(to, op, intRegSP(src.reg()), intRegZR(dst.reg()), offset / 4);
				} else {
					Nat op = intDst ? 0x3E5 : 0x3F5;
					putLoadStoreLargeU(to, op, intRegSP(src.reg()), intRegZR(dst.reg()), offset / 8);
				}

				// Maybe clamp to smaller size.
				if (srcSize > dstSize)
					clampSize(to, dst.reg(), dstSize);
			} else if (src.type() == opRegister) {
				// Make sure that the upper bits are zero. Just move the register if needed, then
				// clamp as necessary.
				if (!same(src.reg(), dst.reg())) {
					regRegMove(to, dst.reg(), src.reg());
				}

				clampSize(to, dst.reg(), dstSize);
			}
		}

		void borOut(Output *to, Instr *instr) {
			Operand src = instr->src();
			Operand dst = instr->dest();
			Nat dstReg = intRegZR(dst.reg());
			bool is64 = dst.size().size64() > 4;
			if (src.type() == opConstant) {
				Word op = src.constant();
				if (op == 0) {
					// Or with zero is a no-op. Don't do anything.
				} else if (allOnes(op, is64)) {
					// Fill target with all ones. We use ORN <dst>, zr, zr for this.
					putData3(to, is64 ? 0x551 : 0x151, dstReg, 31, 31, 0);
				} else {
					putBitmask(to, 0x64, dstReg, dstReg, is64, op);
				}
			} else {
				Nat opCode = is64 ? 0x550 : 0x150;
				putData3(to, opCode, dstReg, dstReg, intRegZR(src.reg()), 0);
			}
		}

		void bandOut(Output *to, Instr *instr) {
			Operand src = instr->src();
			Operand dst = instr->dest();
			Nat dstReg = intRegZR(dst.reg());
			bool is64 = dst.size().size64() > 4;
			if (src.type() == opConstant) {
				Word op = src.constant();
				if (op == 0) {
					// And with zero always gives a zero. Simply emit a mov instruction instead
					// (technically a orr <dst>, zr, zr).
					putData3(to, 0x550, dstReg, 31, 31, 0);
				} else if (allOnes(op, is64)) {
					// And with 0xFF is a no-op. Don't emit any code.
				} else {
					putBitmask(to, 0x24, dstReg, dstReg, is64, op);
				}
			} else {
				Nat opCode = is64 ? 0x450 : 0x050;
				putData3(to, opCode, dstReg, dstReg, intRegZR(src.reg()), 0);
			}
		}

		void testOut(Output *to, Instr *instr) {
			Operand src = instr->src();
			Operand dst = instr->dest();
			Nat dstReg = intRegZR(dst.reg());
			bool is64 = dst.size().size64() > 4;
			if (src.type() == opConstant) {
				Word op = src.constant();
				putBitmask(to, 0xE4, 31, dstReg, is64, op);
			} else {
				Nat opCode = is64 ? 0x750 : 0x350;
				putData3(to, opCode, 31, dstReg, intRegZR(src.reg()), 0);
			}
		}

		void bxorOut(Output *to, Instr *instr) {
			Operand src = instr->src();
			Operand dst = instr->dest();
			Nat dstReg = intRegZR(dst.reg());
			bool is64 = dst.size().size64() > 4;
			if (src.type() == opConstant) {
				Word op = src.constant();
				if (op == 0) {
					// XOR with a zero is a no-op.
				} else if (allOnes(op, is64)) {
					// XOR with all ones is simply a negate. Use orn <dst>, zr, <dst> instead.
					putData3(to, is64 ? 0x551 : 0x151, dstReg, 31, dstReg, 0);
				} else {
					putBitmask(to, 0xA8, dstReg, dstReg, is64, op);
				}
			} else {
				Nat opCode = is64 ? 0x650 : 0x250;
				putData3(to, opCode, dstReg, dstReg, intRegZR(src.reg()), 0);
			}
		}

		void bnotOut(Output *to, Instr *instr) {
			Operand dst = instr->dest();
			Nat dstReg = intRegZR(dst.reg());
			bool is64 = dst.size().size64() > 4;
			// This is ORN <dst>, zr, <dst>
			putData3(to, is64 ? 0x551 : 0x151, dstReg, 31, dstReg, 0);
		}

		void shlOut(Output *to, Instr *instr) {
			Operand src = instr->src();
			Operand dst = instr->dest();
			Nat dstReg = intRegZR(dst.reg());
			bool is64 = dst.size().size64() > 4;
			if (src.type() == opConstant) {
				Nat shift = Nat(src.constant());
				if (shift == 0) {
					// Nothing to do.
				} else if (shift >= Nat(is64 ? 64 : 32)) {
					// Saturated shift. Simply move 0 to the register.
					putData3(to, 0x550, dstReg, 31, 31, 0);
				} else {
					Nat opCode = 0x53000000 | dstReg | (dstReg << 5);
					if (is64)
						opCode |= 0x80400000;

					// immr
					opCode |= ((~shift + 1) & (is64 ? 0x3F : 0x1F)) << 16;
					// imms
					opCode |= (((is64 ? 63 : 31) - shift) & 0x3F) << 10;

					to->putInt(opCode);
				}
			} else {
				putData3(to, is64 ? 0x4D6 : 0x0D6, dstReg, dstReg, intRegZR(src.reg()), 0x08);
			}
		}

		void shrOut(Output *to, Instr *instr) {
			Operand src = instr->src();
			Operand dst = instr->dest();
			Nat dstReg = intRegZR(dst.reg());
			bool is64 = dst.size().size64() > 4;
			if (src.type() == opConstant) {
				Nat shift = Nat(src.constant());
				if (shift == 0) {
					// Nothing to do.
				} else if (shift >= Nat(is64 ? 64 : 32)) {
					// Saturated shift. Simply move 0 to the register.
					putData3(to, 0x550, dstReg, 31, 31, 0);
				} else {
					Nat opCode = 0x53000000 | dstReg | (dstReg << 5);
					if (is64)
						opCode |= 0x80400000;

					// immr
					opCode |= shift << 16;
					// imms
					opCode |= (is64 ? 63 : 31) << 10;

					to->putInt(opCode);
				}
			} else {
				putData3(to, is64 ? 0x4D6 : 0x0D6, dstReg, dstReg, intRegZR(src.reg()), 0x09);
			}
		}

		void sarOut(Output *to, Instr *instr) {
			Operand src = instr->src();
			Operand dst = instr->dest();
			Nat dstReg = intRegZR(dst.reg());
			bool is64 = dst.size().size64() > 4;
			if (src.type() == opConstant) {
				Nat bits = is64 ? 64 : 32;
				Nat shift = Nat(src.constant());
				if (shift > bits - 1)
					shift = bits - 1;
				if (shift == 0) {
					// Nothing to do.
				} else {
					Nat opCode = 0x13000000 | dstReg | (dstReg << 5);
					if (is64)
						opCode |= 0x80400000;

					// immr
					opCode |= shift << 16;
					// imms
					opCode |= bits << 10;

					to->putInt(opCode);
				}
			} else {
				putData3(to, is64 ? 0x4D6 : 0x0D6, dstReg, dstReg, intRegZR(src.reg()), 0x0A);
			}
		}

		void preserveOut(Output *to, Instr *instr) {
			to->markSaved(instr->src().reg(), instr->dest().offset());
		}

		static void fpOut(Output *to, Instr *instr, Nat op) {
			Operand dest = instr->dest();
			Bool is64 = dest.size().size64() > 4;
			Nat baseOp = 0x0F1;
			if (is64)
				baseOp |= 0x2; // sets ftype to 0x1
			Nat destReg = fpReg(dest.reg());
			putData3(to, baseOp, destReg, destReg, fpReg(instr->src().reg()), op);
		}

		void faddOut(Output *to, Instr *instr) {
			fpOut(to, instr, 0x0A);
		}

		void fsubOut(Output *to, Instr *instr) {
			fpOut(to, instr, 0x0E);
		}

		void fnegOut(Output *to, Instr *instr) {
			Operand dest = instr->dest();
			Bool is64 = dest.size().size64() > 4;
			Nat op = 0x1E214000;
			if (is64)
				op |= Nat(1) << 22;
			op |= fpReg(dest.reg());
			op |= fpReg(instr->src().reg()) << 5;
			to->putInt(op);
		}

		void fmulOut(Output *to, Instr *instr) {
			fpOut(to, instr, 0x02);
		}

		void fdivOut(Output *to, Instr *instr) {
			fpOut(to, instr, 0x06);
		}

		void fcmpOut(Output *to, Instr *instr) {
			// Note: This op-code supports comparing to the literal zero. We don't emit that op-code though...
			Operand src = instr->src();
			Operand dest = instr->dest();
			Bool is64 = dest.size().size64() > 4;
			Nat baseOp = 0x0F1;
			if (is64)
				baseOp |= Nat(1) << 1;
			putData3(to, baseOp, 0x0, fpReg(dest.reg()), fpReg(src.reg()), 0x08);
		}

		void fcastOut(Output *to, Instr *instr) {
			Bool in64 = instr->dest().size().size64() > 4;
			Bool out64 = instr->dest().size().size64() > 4;

			if (in64 == out64) {
				// Just emit a mov instruction:
				regRegMove(to, instr->dest().reg(), instr->src().reg());
				return;
			}

			Nat op = 0x1E224000;
			if (in64)
				op |= Nat(1) << 22;
			if (out64)
				op |= Nat(1) << 15;
			op |= intRegZR(instr->dest().reg());
			op |= fpReg(instr->src().reg()) << 5;
			to->putInt(op);
		}

		static void fromFloat(Output *to, Instr *instr, Nat op) {
			Bool in64 = instr->src().size().size64() > 4;
			Bool out64 = instr->dest().size().size64() > 4;

			if (in64)
				op |= Nat(1) << 22;
			if (out64)
				op |= Nat(1) << 31;
			op |= intRegZR(instr->dest().reg());
			op |= fpReg(instr->src().reg()) << 5;
			to->putInt(op);
		}

		void fcastiOut(Output *to, Instr *instr) {
			fromFloat(to, instr, 0x1E380000);
		}

		void fcastuOut(Output *to, Instr *instr) {
			fromFloat(to, instr, 0x1E390000);
		}

		static void toFloat(Output *to, Instr *instr, Nat op) {
			Bool in64 = instr->src().size().size64() > 4;
			Bool out64 = instr->dest().size().size64() > 4;

			if (out64)
				op |= Nat(1) << 22;
			if (in64)
				op |= Nat(1) << 31;
			op |= fpReg(instr->dest().reg());
			op |= intRegZR(instr->src().reg()) << 5;
			to->putInt(op);
		}

		void icastfOut(Output *to, Instr *instr) {
			toFloat(to, instr, 0x1E220000);
		}

		void ucastfOut(Output *to, Instr *instr) {
			toFloat(to, instr, 0x1E230000);
		}

		void datOut(Output *to, Instr *instr) {
			Operand src = instr->src();
			switch (src.type()) {
			case opLabel:
				to->putAddress(src.label());
				break;
			case opReference:
				to->putAddress(src.ref());
				break;
			case opObjReference:
				to->putObject(src.object());
				break;
			case opConstant:
				to->putSize(src.constant(), src.size());
				break;
			default:
				assert(false, L"Unsupported type for 'dat'.");
				break;
			}
		}

		void lblOffsetOut(Output *to, Instr *instr) {
			to->putOffset(instr->src().label());
		}

		void alignOut(Output *to, Instr *instr) {
			to->align(Nat(instr->src().constant()));
		}

		void locationOut(Output *, Instr *) {
			// We don't save location data in the generated code.
		}

		void metaOut(Output *, Instr *) {
			// We don't save metadata in the generated code.
		}

#define OUTPUT(x) { op::x, &x ## Out }

		typedef void (*OutputFn)(Output *to, Instr *instr);

		// Note: "mov" is special: we try to merge mov operations.
		const OpEntry<OutputFn> outputMap[] = {
			OUTPUT(nop),
			OUTPUT(prolog),
			OUTPUT(epilog),
			OUTPUT(mov),
			OUTPUT(lea),
			OUTPUT(call),
			OUTPUT(ret),
			OUTPUT(jmp),
			OUTPUT(sub),
			OUTPUT(add),
			OUTPUT(cmp),
			OUTPUT(setCond),
			OUTPUT(mul),
			OUTPUT(idiv),
			OUTPUT(udiv),
			OUTPUT(icast),
			OUTPUT(ucast),
			OUTPUT(band),
			OUTPUT(bor),
			OUTPUT(bxor),
			OUTPUT(bnot),
			OUTPUT(test),
			OUTPUT(shl),
			OUTPUT(shr),
			OUTPUT(sar),

			OUTPUT(fadd),
			OUTPUT(fsub),
			OUTPUT(fneg),
			OUTPUT(fmul),
			OUTPUT(fdiv),
			OUTPUT(fcmp),
			OUTPUT(fcast),
			OUTPUT(fcasti),
			OUTPUT(fcastu),
			OUTPUT(icastf),
			OUTPUT(ucastf),

			OUTPUT(preserve),

			OUTPUT(dat),
			OUTPUT(lblOffset),
			OUTPUT(align),
			OUTPUT(location),
			OUTPUT(meta),
		};

		bool empty(Array<Label> *x) {
			return x == null || x->empty();
		}

		enum MergeResult {
			mergeNone,
			mergeSimple,
			mergePreserve,
		};

		bool anyLabels(Listing *src, Nat at) {
			Array<Label> *lbl = src->labels(at);
			return lbl != null && lbl->count() > 0;
		}

		MergeResult mergeMov(Listing *src, Output *to, Instr *first, Nat at) {
			Nat remaining = src->count() - at;

			// Note: We can't merge loads if there are labels between. Otherwise, we can't jump to
			// those labels later on.
			if (remaining >= 1 && !anyLabels(src, at + 1)) {
				// Immediately followed by a mov?
				Instr *second = src->at(at + 1);
				if (second->op() == op::mov) {
					return movOut(to, first, second) ? mergeSimple : mergeNone;
				}

				// Maybe a "preserve" is between?
				if (remaining >= 2 && second->op() == op::preserve && !anyLabels(src, at + 2)) {
					Instr *third = src->at(at + 2);
					if (third->op() == op::mov) {
						return movOut(to, first, third) ? mergePreserve : mergeNone;
					}
				}
			}

			// If nothing else, just emit a regular mov.
			movOut(to, first);
			return mergeNone;
		}

		void output(Listing *src, Output *to) {
			static OpTable<OutputFn> t(outputMap, ARRAY_COUNT(outputMap));

			// TODO: Combination of mov + add/sub would be nice to combine if possible.

			for (Nat i = 0; i < src->count(); i++) {
				to->mark(src->labels(i));

				Instr *instr = src->at(i);
				op::OpCode opCode = instr->op();
				if (opCode == op::mov) {
					// We can merge "mov" ops into ldp and stp sometimes. If we find mov ops
					// (possibly separated by "preserve" pseudo-ops), we can try to merge them.
					MergeResult r = mergeMov(src, to, instr, i);
					if (r == mergeSimple) {
						// Consumed 2 ops.
						i++;
					} else if (r == mergePreserve) {
						// Consumed 3 ops, but we need to handle the "preserve" now (out of order).
						preserveOut(to, src->at(i + 1));
						i += 2;
					}
				} else {
					// Regular path.
					OutputFn fn = t[instr->op()];
					if (fn) {
						(*fn)(to, instr);
					} else {
						assert(false, L"Unsupported op-code: " + String(name(instr->op())));
					}
				}
			}

			to->mark(src->labels(src->count()));
		}

	}
}
