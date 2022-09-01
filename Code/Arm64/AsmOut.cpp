#include "stdafx.h"
#include "AsmOut.h"
#include "Asm.h"
#include "../OpTable.h"
#include "../Exception.h"

namespace code {
	namespace arm64 {

		// Good reference for instruction encoding:
		// https://developer.arm.com/documentation/ddi0596/2021-12/Index-by-Encoding?lang=en

		// Debug instruction in big endian.
		void debugInstr(Nat op) {
			PLN(L"./disas_arm64.sh " << toHex(Byte(op & 0xFF))
				<< L" " << toHex(Byte((op >> 8) & 0xFF))
				<< L" " << toHex(Byte((op >> 16) & 0xFF))
				<< L" " << toHex(Byte((op >> 24) & 0xFF)));
		}

		// Put data instructions. 3 registers, and a 6-bit immediate.
		void putData(Output *to, Nat op, Nat rDest, Nat ra, Nat rb, Nat imm) {
			Nat instr = (op << 21) | rDest | (ra << 16) | (rb << 5) | ((imm & 0x3F) << 10);
			debugInstr(instr);
			to->putInt(instr);
		}

		// Put data instructions. 2 registers, 12-bit immediate.
		void putData(Output *to, Nat op, nat rDest, Nat rSrc, Nat imm) {
			Nat instr = (op << 22) | rDest | (rSrc << 5) | ((imm & 0xFFF) << 10);
			debugInstr(instr);
			to->putInt(instr);
		}

		// Put instructions for loads and stores: 3 registers and a 7-bit immediate.
		void putLoadStore(Output *to, Nat op, Nat base, Nat r1, Nat r2, Nat imm) {
			Nat instr = (op << 22) | r1 | (base << 5) | (r2 << 10) | ((imm & 0x7F) << 15);
			debugInstr(instr);
			to->putInt(instr);
		}

		// Put a "large" load/store (for bytes, mainly): 2 registers and 12-bit immediate.
		void putLoadStoreLarge(Output *to, Nat op, Nat base, Nat r1, Nat imm) {
			Nat instr = (op << 22) | r1 | (base << 5) | ((0xFFF & imm) << 10);
			debugInstr(instr);
			to->putInt(instr);
		}

		// Check if value fits in 7-bit signed.
		static Bool isImm7(Int value) {
			return value >= -0x40 && value <= 0x3F;
		}

		// Check if value fits in 12-bit signed.
		static Bool isImm12(Int value) {
			return value >= -0x800 && value <= 0x7FF;
		}

		bool prologOut(Output *to, Instr *instr, Instr *) {
			Offset stackSize = instr->src().offset();
			PVAR(stackSize);
			Int scaled = stackSize.v64() / 8;
			if (!isImm7(scaled)) {
				TODO(L"Make prolog that handles this properly.");
				throw new (to) InvalidValue(S("Too large stack size for Arm64!"));
			}

			// We emit:
			// - stp x29, x30, [sp, -stackSize]!
			putLoadStore(to, 0x2A6, 31, 29, 30, -scaled);
			// - mov x29, sp   # create stack frame
			putData(to, 0x244, 29, 31, 0);

			// TODO: Output DWARF metadata as well.
			return false;
		}

		bool epilogOut(Output *to, Instr *instr, Instr *) {
			Offset stackSize = instr->src().offset();
			Int scaled = stackSize.v64() / 8;
			if (isImm7(scaled)) {
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
			return false;
		}

		bool loadOut(Output *to, Instr *instr, Instr *next) {
			Reg baseReg = instr->src().reg();
			Int offset = instr->src().offset().v64();
			Int opSize = instr->dest().size().size64();
			Reg dest1 = instr->dest().reg();
			Reg dest2 = noReg;

			// Bytes are special:
			if (opSize == 1) {
				if (!isImm12(offset))
					throw new (to) InvalidValue(S("Too large offset!"));
				putLoadStoreLarge(to, 0x0E5, intRegNumber(baseReg), intRegNumber(dest1), offset);
				return false;
			}

			// Look at "next" to see if we can merge it with this instruction.
			if (next->op() == op::mov && next->dest().type() == opRegister && next->src().type() == opRelative) {
				if (same(next->src().reg(), baseReg) && Int(next->dest().size().size64()) == opSize) {
					// Look at the offsets, if they are next to each other, we can merge them.
					Int off = next->src().offset().v64();
					if (off == offset + opSize && isImm7(offset / opSize)) {
						dest2 = next->dest().reg();
					} else if (off == offset - opSize && isImm7(off / opSize)) {
						// Put the second one first.
						dest2 = dest1;
						dest1 = next->dest().reg();
						offset = off;
					}
				}
			}

			if (offset % opSize)
				throw new (to) InvalidValue(S("Memory access on Arm must be aligned!"));
			offset /= opSize;

			// TODO: Handle fp registers!
			if (dest2 != noReg) {
				// Note: Should not happen if merging code above is correct.
				if (!isImm7(offset))
					throw new (to) InvalidValue(S("Too large offset!"));

				Nat op = opSize == 4 ? 0x0A5 : 0x2A5;
				putLoadStore(to, op, intRegNumber(baseReg), intRegNumber(dest1), intRegNumber(dest2), offset);
			} else {
				if (!isImm12(offset))
					throw new (to) InvalidValue(S("Too large offset!"));

				Nat op = opSize == 4 ? 0x2E5 : 0x3E5;
				putLoadStoreLarge(to, op, intRegNumber(baseReg), intRegNumber(dest1), offset);
			}

			return dest2 != noReg;
		}

		bool storeOut(Output *to, Instr *instr, Instr *next) {
			Reg baseReg = instr->dest().reg();
			Int offset = instr->dest().offset().v64();
			Int opSize = instr->src().size().size64();
			Reg src1 = instr->src().reg();
			Reg src2 = noReg;

			// Bytes are special:
			if (opSize == 1) {
				if (!isImm12(offset))
					throw new (to) InvalidValue(S("Too large offset!"));
				putLoadStoreLarge(to, 0x0E4, intRegNumber(baseReg), intRegNumber(src1), offset);
				return false;
			}

			// Look at "next" to see if we can merge it with this instruction.
			if (next->op() == op::mov && next->src().type() == opRegister && next->dest().type() == opRelative) {
				if (same(next->dest().reg(), baseReg) && Int(next->src().size().size64()) == opSize) {
					// Look at the offsets, if they are next to each other, we can merge them.
					Int off = next->dest().offset().v64();
					if (off == offset + opSize && isImm7(offset / opSize)) {
						src2 = next->src().reg();
					} else if (off == offset - opSize && isImm7(off / opSize)) {
						// Put the second one first.
						src2 = src1;
						src1 = next->src().reg();
						offset = off;
					}
				}
			}

			if (offset % opSize)
				throw new (to) InvalidValue(S("Memory access on Arm must be aligned!"));
			offset /= opSize;

			// TODO: Handle fp registers!
			if (src2 != noReg) {
				// Note: Should not happen if merging code above is correct.
				if (!isImm7(offset))
					throw new (to) InvalidValue(S("Too large offset!"));

				Nat op = opSize == 4 ? 0x0A4 : 0x2A4;
				putLoadStore(to, op, intRegNumber(baseReg), intRegNumber(src1), intRegNumber(src2), offset);
			} else {
				if (!isImm12(offset))
					throw new (to) InvalidValue(S("Too large offset!"));

				Nat op = opSize == 4 ? 0x2E4 : 0x3E4;
				putLoadStoreLarge(to, op, intRegNumber(baseReg), intRegNumber(src1), offset);
			}

			return src2 != noReg;
		}

		void regRegMove(Output *to, Reg dest, Reg src) {
			Bool intSrc = isIntReg(src);
			Bool intDst = isIntReg(dest);
			if (intSrc && intDst) {
				if (size(src).size64() > 4)
					putData(to, 0x560, intRegNumber(dest), intRegNumber(src), 31);
				else
					putData(to, 0x160, intRegNumber(dest), intRegNumber(src), 31);
			} else {
				assert(false, L"Mov to/from fp registers is not yet implemented!");
			}
		}

		bool movOut(Output *to, Instr *instr, Instr *next) {
			PVAR(instr); PVAR(next);
			switch (instr->src().type()) {
			case opRegister:
				// Fall thru to below.
				break;
			case opRelative:
				return loadOut(to, instr, next);

				// TODO: More, for example:
				// case opConstant:
			default:
				assert(false, L"Unsupported mov src operand.");
				return false;
			}

			// "src" is now a register. Examine "dest".

			switch (instr->dest().type()) {
			case opRegister:
				regRegMove(to, instr->dest().reg(), instr->src().reg());
				return false;
			case opRelative:
				return storeOut(to, instr, next);
			default:
				assert(false, L"Unsupported mov destination.");
				return false;
			}
		}

		bool datOut(Output *to, Instr *instr, Instr *) {
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

			return false;
		}

		bool alignOut(Output *to, Instr *instr, Instr *) {
			to->align(Nat(instr->src().constant()));
			return false;
		}

#define OUTPUT(x) { op::x, &x ## Out }

		// This is a bit special compared to other backends:
		// To allow minimal peephole optimization when generating code, the next instruction
		// is provided as well. If "next" is non-null, then there is no label between the
		// instructions, so we can merge them. If the output chose to merge instructions,
		// the output function should return "true".
		typedef bool (*OutputFn)(Output *to, Instr *instr, MAYBE(Instr *) next);

		const OpEntry<OutputFn> outputMap[] = {
			OUTPUT(prolog),
			OUTPUT(epilog),
			OUTPUT(mov),

			OUTPUT(dat),
			OUTPUT(align),
		};

		bool empty(Array<Label> *x) {
			return x == null || x->empty();
		}

		void output(Listing *src, Output *to) {
			static OpTable<OutputFn> t(outputMap, ARRAY_COUNT(outputMap));

			// TODO: Handle cases where we have "store" ops between stores. Strip them out when passing to ops.

			for (Nat i = 0; i < src->count(); i++) {
				to->mark(src->labels(i));

				Instr *instr = src->at(i);
				OutputFn fn = t[instr->op()];
				if (fn) {
					Instr *next = null;
					if (i + 1 < src->count() && empty(src->labels(i + 1)))
						next = src->at(i + 1);

					if ((*fn)(to, instr, next))
						i++;
				} else {
					assert(false, L"Unsupported op-code: " + String(name(instr->op())));
				}
			}

			to->mark(src->labels(src->count()));
		}

	}
}
