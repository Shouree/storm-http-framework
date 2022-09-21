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
			assert(false, L"Can not use this register with this op-code.");
		}

		// Get register number where 31=zr.
		static Nat intRegZR(Reg reg) {
			Nat r = intRegNumber(reg);
			if (r < 32)
				return r;
			assert(false, L"Can not use this register with this op-code.");
		}

		// Get fp register number.
		static Nat fpReg(Reg reg) {
			return vectorRegNumber(reg);
		}

		// Good reference for instruction encoding:
		// https://developer.arm.com/documentation/ddi0596/2021-12/Index-by-Encoding?lang=en

		// Check if value fits in 6-bit signed.
		static Bool isImm6S(Int value) {
			return value >= -0x20 && value <= 0x1F;
		}
		static void checkImm6S(RootObject *e, Int value) {
			if (!isImm6S(value))
				throw new (e) InvalidValue(TO_S(e, S("Too large signed 6-bit immediate value: ") << value));
		}

		// Check if value fits in 6-bit unsigned.
		static Bool isImm6U(Nat value) {
			return value <= 0x3F;
		}
		static void checkImm6U(RootObject *e, Int value) {
			if (!isImm6U(value))
				throw new (e) InvalidValue(TO_S(e, S("Too large unsigned 6-bit immediate value: ") << value));
		}

		// Check if value fits in 7-bit signed.
		static Bool isImm7S(Int value) {
			return value >= -0x40 && value <= 0x3F;
		}
		static void checkImm7S(RootObject *e, Int value) {
			if (!isImm7S(value))
				throw new (e) InvalidValue(TO_S(e, S("Too large signed 7-bit immediate value: ") << value));
		}

		// Check if value fits in 7-bit unsigned.
		static Bool isImm7U(Nat value) {
			return value <= 0x7F;
		}
		static void checkImm7U(RootObject *e, Nat value) {
			if (!isImm7U(value))
				throw new (e) InvalidValue(TO_S(e, S("Too large unsigned 7-bit immediate value: ") << value));
		}

		// Check if value fits in 12-bit signed.
		static Bool isImm12S(Int value) {
			return value >= -0x800 && value <= 0x7FF;
		}
		static void checkImm12S(RootObject *e, Int value) {
			if (!isImm12S(value))
				throw new (e) InvalidValue(TO_S(e, S("Too large signed 12-bit immediate value: ") << value));
		}

		// Check if value fits in 12-bit unsigned.
		static Bool isImm12U(Nat value) {
			return value <= 0xFFF;
		}
		static void checkImm12U(RootObject *e, Nat value) {
			if (!isImm12U(value))
				throw new (e) InvalidValue(TO_S(e, S("Too large unsigned 12-bit immediate value: ") << value));
		}

		// Check if value fits in 19-bit signed.
		static Bool isImm19S(Int value) {
			return value >= -0x40000 && value <= 0x3FFFF;
		}
		static void checkImm19S(RootObject *e, Int value) {
			if (!isImm19S(value))
				throw new (e) InvalidValue(TO_S(e, S("Too large signed 19-bit immediate value: ") << value));
		}

		// Check if value fits in 19-bit unsigned.
		static Bool isImm19U(Nat value) {
			return value <= 0x7FFFF;
		}
		static void checkImm19U(RootObject *e, Int value) {
			if (!isImm19U(value))
				throw new (e) InvalidValue(TO_S(e, S("Too large unsigned 19-bit immediate value: ") << value));
		}

		// Put data instructions. 3 registers, and a 6-bit unsigned immediate.
		static inline void putData(Output *to, Nat op, Nat rDest, Nat ra, Nat rb, Nat imm) {
			checkImm6U(to, imm);
			Nat instr = (op << 21) | rDest | (ra << 16) | (rb << 5) | ((imm & 0x3F) << 10);
			to->putInt(instr);
		}

		// Put data instructions. 2 registers, 12-bit unsigned immediate.
		static inline void putData(Output *to, Nat op, nat rDest, Nat rSrc, Nat imm) {
			checkImm12U(to, imm);
			Nat instr = (op << 22) | rDest | (rSrc << 5) | ((imm & 0xFFF) << 10);
			to->putInt(instr);
		}

		// Put instructions for loads and stores: 3 registers and a 7-bit signed immediate.
		static inline void putLoadStore(Output *to, Nat op, Nat base, Nat r1, Nat r2, Int imm) {
			checkImm7S(to, imm);
			Nat instr = (op << 22) | r1 | (base << 5) | (r2 << 10) | ((imm & 0x7F) << 15);
			to->putInt(instr);
		}

		// Put a "large" load/store (for bytes, mainly): 2 registers and 12-bit immediate.
		static inline void putLoadStoreLarge(Output *to, Nat op, Nat base, Nat r1, Int imm) {
			checkImm12S(to, imm);
			Nat instr = (op << 22) | r1 | (base << 5) | ((0xFFF & imm) << 10);
			to->putInt(instr);
		}

		// Put a load/store with 19-bit immediate offset from PC.
		static inline void putLoadStoreImm(Output *to, Nat op, Nat reg, Int imm) {
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

		void prologOut(Output *to, Instr *instr) {
			Offset stackSize = instr->src().offset();
			Int scaled = stackSize.v64() / 8;
			if (!isImm7S(scaled)) {
				TODO(L"Make prolog that handles this properly.");
				throw new (to) InvalidValue(S("Too large stack size for Arm64!"));
			}

			// We emit:
			// - stp x29, x30, [sp, -stackSize]!
			putLoadStore(to, 0x2A6, 31, 29, 30, -scaled);
			// - mov x29, sp   # create stack frame
			putData(to, 0x244, 29, 31, 0);

			// TODO: Output DWARF metadata as well.
		}

		void epilogOut(Output *to, Instr *instr) {
			Offset stackSize = instr->src().offset();
			Int scaled = stackSize.v64() / 8;
			if (!isImm7S(scaled)) {
				TODO(L"Make epilog that handles this properly.");
				throw new (to) InvalidValue(S("Too large stack size for Arm64!"));
			}
			// We emit:
			// - ldp x29, x30, [sp], stackSize
			putLoadStore(to, 0x2A3, 31, 29, 30, scaled);

			// Note: We could also emit:
			// - ldp x29, x30, [x29]
			// - add sp, sp, #stackSize
			// this is more robust against messing up SP, but uses more instructions, and is likely slower.

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
				Nat op = intReg ? 0x0E5 : 0x0F5;
				putLoadStoreLarge(to, op, intRegSP(baseReg), intRegZR(dest1), offset);
				return false;
			}

			// Look at "next" to see if we can merge it with this instruction.
			if (next && next->dest().type() == opRegister && next->src().type() == opRelative) {
				if (same(next->src().reg(), baseReg) && Int(next->dest().size().size64()) == opSize) {
					if (isIntReg(next->dest().reg()) == intReg) {
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

			if (dest2 != noReg) {
				Nat op;
				if (intReg)
					op = opSize == 4 ? 0x0A5 : 0x2A5;
				else
					op = opSize == 4 ? 0x0B5 : 0x1B5;
				putLoadStore(to, op, intRegSP(baseReg), intRegZR(dest1), intRegZR(dest2), offset);
			} else {
				Nat op = 0;
				if (intReg)
					op = opSize == 4 ? 0x2E5 : 0x3E5;
				else
					op = opSize == 4 ? 0x2F5 : 0x3E5;
				putLoadStoreLarge(to, op, intRegSP(baseReg), intRegZR(dest1), offset);
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
				Nat op = intReg ? 0x0E4 : 0x0F4;
				putLoadStoreLarge(to, op, intRegSP(baseReg), intRegZR(src1), offset);
				return false;
			}

			// Look at "next" to see if we can merge it with this instruction.
			if (next && next->src().type() == opRegister && next->dest().type() == opRelative) {
				if (same(next->dest().reg(), baseReg) && Int(next->src().size().size64()) == opSize) {
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

			// TODO: Handle fp registers!
			if (src2 != noReg) {
				Nat op;
				if (intReg)
					op = opSize == 4 ? 0x0A4 : 0x2A4;
				else
					op = opSize == 4 ? 0x0B4 : 0x1B4;
				putLoadStore(to, op, intRegSP(baseReg), intRegZR(src1), intRegZR(src2), offset);
			} else {
				Nat op;
				if (intReg)
					op = opSize == 4 ? 0x2E4 : 0x3E4;
				else
					op = opSize == 4 ? 0x2F4 : 0x3F4;
				putLoadStoreLarge(to, op, intRegSP(baseReg), intRegZR(src1), offset);
			}

			return src2 != noReg;
		}

		void regRegMove(Output *to, Reg dest, Reg src) {
			Bool intSrc = isIntReg(src);
			Bool intDst = isIntReg(dest);
			if (intSrc && intDst) {
				if (size(src).size64() > 4)
					putData(to, 0x550, intRegZR(dest), intRegZR(src), 31, 0);
				else
					putData(to, 0x150, intRegZR(dest), intRegZR(src), 31, 0);
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
					throw new (to) InvalidValue(S("Invalid source for store operation on ARM."));
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
				if (src.constant() >= 0xFFFF)
					throw new (to) InvalidValue(S("Too large immediate to load"));
				// Note: No difference between nat and word version.
				to->putInt(0xD2800000 | ((0xFFFF & src.constant()) << 5) | intRegZR(destReg));
				return false;
			case opLabel:
			case opRelativeLbl: {
				Int offset = to->offset(src.label()) + src.offset().v64() - to->tell();
				putLoadStoreImm(to, 0x58, intRegZR(destReg), offset / 4);
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
				throw new (to) InvalidValue(S("Destination of lea should have bbeen transformed to a register."));
			Nat dest = intRegZR(destOp.reg());

			Operand src = instr->src();
			switch (src.type()) {
			case opRelative:
				// Note: These are not sign-extended, so we need to be careful about the sign.
				if (src.offset().v64() > 0) {
					// add:
					putData(to, 0x244, dest, intRegZR(src.reg()), src.offset().v64());
				} else {
					// sub:
					putData(to, 0x344, dest, intRegZR(src.reg()), -src.offset().v64());
				}
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
				// Split into two op-codes: a load and a register call.
				offset = target.offset().v64() / 8;
				putLoadStoreLarge(to, 0x3E5, intRegZR(target.reg()), 17, offset);
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
			// Note: Conditional jumps are limited on Arm to only use imm19.
			assert(false, L"Implement me!");
		}

		void jmpOut(Output *to, Instr *instr) {
			CondFlag cond = instr->src().condFlag();
			Operand target = instr->dest();
			Nat offset;

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
				putLoadStoreLarge(to, 0x3E5, intRegZR(target.reg()), 17, offset);
				// br x17
				to->putInt(0xD61F0220);
				break;
			default:
				PVAR(target);
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
				putData(to, opReg, dest, intRegSP(instr->src().reg()), dest, 0);
				break;
			case opConstant:
				putData(to, opImm, dest, dest, instr->src().constant());
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
				putData(to, opReg, 31, intRegSP(instr->src().reg()), dest, 0);
				break;
			case opConstant:
				putData(to, opImm, 31, dest, instr->src().constant());
				break;
			default:
				assert(false, L"Unsupported source for data operation.");
				break;
			}
		}

		void setCondOut(Output *to, Instr *instr) {
			Nat dest = intRegZR(instr->dest().reg());
			Nat opCode = 0x1A9F07E0 | dest | (condArm64(inverse(instr->src().condFlag())) << 12);
			to->putInt(opCode);
		}

		void preserveOut(Output *to, Instr *instr) {
			TODO(L"Implement PRESERVE pseudo-op!");
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

#define OUTPUT(x) { op::x, &x ## Out }

		typedef void (*OutputFn)(Output *to, Instr *instr);

		// Note: "mov" is special: we try to merge mov operations.
		const OpEntry<OutputFn> outputMap[] = {
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

			OUTPUT(preserve),

			OUTPUT(dat),
			OUTPUT(lblOffset),
			OUTPUT(align),
		};

		bool empty(Array<Label> *x) {
			return x == null || x->empty();
		}

		enum MergeResult {
			mergeNone,
			mergeSimple,
			mergePreserve,
		};

		MergeResult mergeMov(Listing *src, Output *to, Instr *first, Nat at) {
			Nat remaining = src->count() - at;

			if (remaining >= 1) {
				// Immediately followed by a mov?
				Instr *second = src->at(at + 1);
				if (second->op() == op::mov) {
					return movOut(to, first, second) ? mergeSimple : mergeNone;
				}

				// Maybe a "preserve" is between?
				if (remaining >= 2 && second->op() == op::preserve) {
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
						preserveOut(to, src->at(i + 2));
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
