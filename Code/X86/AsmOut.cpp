#include "stdafx.h"
#include "AsmOut.h"
#include "Asm.h"
#include "OpTable.h"

namespace code {
	namespace x86 {

		void epilogOut(Output *to, Instr *instr) {
			(void)instr;
			// This is LEAVE.
			to->putByte(0xC9);
		}

		static void movFromFp(Output *to, Instr *instr) {
			Operand dst = instr->dest();
			bool large = dst.size().size32() > 4;
			bool spilled = false;
			if (dst.type() == opRegister) {
				// Need to spill to memory.
				spilled = true;
				dst = xRel(dst.size(), ptrStack, Offset());
				// Reserve memory on stack (esp -= 4/8)
				to->putByte(0x81);
				modRm(to, 5, ptrStack);
				to->putInt(dst.size().size32());
			}

			if (large)
				to->putByte(0xF2);
			else
				to->putByte(0xF3);
			to->putByte(0x0F);
			to->putByte(0x11);
			modRm(to, fpRegisterId(instr->src().reg()), dst);

			if (spilled) {
				dst = instr->dest();
				if (large) {
					to->putByte(0x58 + registerId(low32(dst.reg())));
					to->putByte(0x58 + registerId(high32(dst.reg())));
				} else {
					to->putByte(0x58 + registerId(dst.reg()));
				}
			}
		}

		static void movToFp(Output *to, Instr *instr) {
			Operand src = instr->src();
			bool large = src.size().size32() > 4;
			bool spilled = false;
			if (src.type() == opRegister) {
				// Spill to memory using push.
				spilled = true;
				if (large) {
					to->putByte(0x50 + registerId(high32(src.reg())));
					to->putByte(0x50 + registerId(low32(src.reg())));
				} else {
					to->putByte(0x50 + registerId(src.reg()));
				}
				src = xRel(src.size(), ptrStack, Offset());
			}

			if (large)
				to->putByte(0xF2);
			else
				to->putByte(0xF3);
			to->putByte(0x0F);
			to->putByte(0x10);
			modRm(to, fpRegisterId(instr->dest().reg()), src);

			if (spilled) {
				// Clear stack (esp += 4/8)
				to->putByte(0x81);
				modRm(to, 0, ptrStack);
				to->putInt(src.size().size32());
			}
		}

		static void movOut(Output *to, const Operand &dest, const Operand &src) {
			// Integer mov.
			ImmRegInstr8 op8 = {
				0xC6, 0,
				0x88,
				0x8A,
			};
			ImmRegInstr op = {
				0x0, 0xFF, // Not supported
				0xC7, 0,
				0x89,
				0x8B,
			};
			immRegInstr(to, op8, op, dest, src);
		}

		void movOut(Output *to, Instr *instr) {
			bool fpSrc = fpRegister(instr->src());
			bool fpDest = fpRegister(instr->dest());
			if (fpSrc && fpDest) {
				// Two fp registers:
				if (instr->size() == Size::sDouble)
					to->putByte(0xF2);
				else
					to->putByte(0xF3);
				to->putByte(0x0F);
				to->putByte(0x10);
				modRm(to, fpRegisterId(instr->dest().reg()), instr->src());
			} else if (fpSrc) {
				movFromFp(to, instr);
			} else if (fpDest) {
				movToFp(to, instr);
			} else {
				movOut(to, instr->dest(), instr->src());
			}
		}

		void swapOut(Output *to, Instr *instr) {
			if (instr->size() == Size::sByte) {
				to->putByte(0x86);
			} else {
				to->putByte(0x87);
			}
			modRm(to, registerId(instr->dest().reg()), instr->src());
		}

		static void addOut(Output *to, const Operand &dest, const Operand &src) {
			ImmRegInstr8 op8 = {
				0x80, 0,
				0x00,
				0x02,
			};
			ImmRegInstr op = {
				0x83, 0,
				0x81, 0,
				0x01,
				0x03
			};
			immRegInstr(to, op8, op, dest, src);
		}
		void addOut(Output *to, Instr *instr) {
			addOut(to, instr->dest(), instr->src());
		}

		static void adcOut(Output *to, const Operand &dest, const Operand &src) {
			ImmRegInstr8 op8 = {
				0x80, 2,
				0x10,
				0x12
			};
			ImmRegInstr op = {
				0x83, 2,
				0x81, 2,
				0x11,
				0x13
			};
			immRegInstr(to, op8, op, dest, src);
		}
		void adcOut(Output *to, Instr *instr) {
			adcOut(to, instr->dest(), instr->src());
		}

		void borOut(Output *to, Instr *instr) {
			ImmRegInstr8 op8 = {
				0x80, 1,
				0x08,
				0x0A
			};
			ImmRegInstr op = {
				0x83, 1,
				0x81, 1,
				0x09,
				0x0B
			};
			immRegInstr(to, op8, op, instr->dest(), instr->src());
		}

		void bandOut(Output *to, Instr *instr) {
			ImmRegInstr8 op8 = {
				0x80, 4,
				0x20,
				0x22
			};
			ImmRegInstr op = {
				0x83, 4,
				0x81, 4,
				0x21,
				0x23
			};
			immRegInstr(to, op8, op, instr->dest(), instr->src());
		}

		void bnotOut(Output *to, Instr *instr) {
			const Operand &dest = instr->dest();
			if (dest.size() == Size::sByte) {
				to->putByte(0xF6);
				modRm(to, 2, dest);
			} else if (dest.size() == Size::sInt || dest.size() == Size::sPtr) {
				to->putByte(0xF7);
				modRm(to, 2, dest);
			} else {
				assert(false, L"Unsupported size for 'not'");
			}
		}

		void subOut(Output *to, Instr *instr) {
			ImmRegInstr8 op8 = {
				0x80, 5,
				0x28,
				0x2A
			};
			ImmRegInstr op = {
				0x83, 5,
				0x81, 5,
				0x29,
				0x2B
			};
			immRegInstr(to, op8, op, instr->dest(), instr->src());
		}

		void sbbOut(Output *to, Instr *instr) {
			ImmRegInstr8 op8 = {
				0x80, 2,
				0x18,
				0x1A
			};
			ImmRegInstr op = {
				0x83, 2,
				0x81, 2,
				0x19,
				0x1B
			};
			immRegInstr(to, op8, op, instr->dest(), instr->src());
		}

		static void bxorOut(Output *to, const Operand &dest, const Operand &src) {
			ImmRegInstr8 op8 = {
				0x80, 6,
				0x30,
				0x32
			};
			ImmRegInstr op = {
				0x83, 6,
				0x81, 6,
				0x31,
				0x33
			};
			immRegInstr(to, op8, op, dest, src);
		}
		void bxorOut(Output *to, Instr *instr) {
			bxorOut(to, instr->dest(), instr->src());
		}

		void cmpOut(Output *to, Instr *instr) {
			ImmRegInstr8 op8 = {
				0x80, 7,
				0x38,
				0x3A
			};
			ImmRegInstr op = {
				0x83, 7,
				0x81, 7,
				0x39,
				0x3B
			};
			immRegInstr(to, op8, op, instr->dest(), instr->src());
		}

		void testOut(Output *to, Instr *instr) {
			ImmRegInstr8 op8 = {
				0xF6, 7,
				0x84,
				0x84 // TEST is symmetric
			};
			ImmRegInstr op = {
				0x0, 0xFF, // Not supported
				0xF7, 0,
				0x85,
				0x85 // TEST is symmetric
			};
			immRegInstr(to, op8, op, instr->dest(), instr->src());
		}

		void pushOut(Output *to, Instr *instr) {
			const Operand &src = instr->src();
			assert(src.size() != Size::sLong);

			switch (src.type()) {
			case opConstant:
				if (singleByte(src.constant())) {
					to->putByte(0x6A);
					to->putByte(Byte(src.constant() & 0xFF));
				} else {
					to->putByte(0x68);
					to->putInt(Int(src.constant()));
				}
				break;
			case opRegister:
				to->putByte(0x50 + registerId(src.reg()));
				break;
			case opRelative:
				to->putByte(0xFF);
				modRm(to, 6, src);
				break;
			case opReference:
				to->putByte(0x68);
				to->putAddress(src.ref());
				break;
			case opObjReference:
				to->putByte(0x68);
				to->putObject(src.object());
				break;
			case opLabel:
				to->putByte(0x68);
				to->putAddress(src.label());
				break;
			default:
				assert(false, L"Push does not support this operand type.");
				break;
			}
		}

		void popOut(Output *to, Instr *instr) {
			const Operand &dest = instr->dest();
			assert(dest.size() != Size::sLong);

			switch (dest.type()) {
			case opRegister:
				to->putByte(0x58 + registerId(dest.reg()));
				break;
			case opRelative:
				to->putByte(0x8F);
				modRm(to, 0, dest);
				break;
			default:
				assert(false, L"Pop does not support this operand type.");
			}
		}

		void pushFlagsOut(Output *to, Instr *instr) {
			to->putByte(0x9C);
		}

		void popFlagsOut(Output *to, Instr *instr) {
			to->putByte(0x9D);
		}

		void retOut(Output *to, Instr *instr) {
			to->putByte(0xC3);
		}

		void setCondOut(Output *to, Instr *instr) {
			CondFlag c = instr->src().condFlag();
			to->putByte(0x0F);
			to->putByte(0x90 + condOp(c));
			modRm(to, 0, instr->dest());
		}

		static void jmpCall(byte opCode, Output *to, const Operand &src) {
			switch (src.type()) {
			case opConstant:
				// Used in the 64-bit transformation, which is actually safe.
				// WARNING(L"Jumping to a constant is not good!");
				to->putByte(opCode);
				to->putRelativeStatic(src.constant());
				break;
			case opLabel:
				to->putByte(opCode);
				to->putRelative(src.label());
				break;
			case opReference:
				to->putByte(opCode);
				to->putRelative(src.ref());
				break;
			default:
				assert(false, L"JmpCall not implemented for " + ::toS(src));
				break;
			}
		}

		static void jmpCall(bool call, Output *to, const Operand &src) {
			switch (src.type()) {
			case opConstant:
			case opLabel:
			case opReference:
				jmpCall(byte(call ? 0xE8 : 0xE9), to, src);
				break;
			case opRegister:
			case opRelative:
				to->putByte(0xFF);
				modRm(to, call ? 2 : 4, src);
				break;
			default:
				assert(false, L"JmpCall not implemented for " + ::toS(src));
				break;
			}
		}

		void jmpOut(Output *to, Instr *instr) {
			CondFlag c = instr->src().condFlag();
			if (c == ifAlways) {
				jmpCall(false, to, instr->dest());
			} else if (c == ifNever) {
				// Nothing.
			} else {
				byte op = 0x80 + condOp(c);
				to->putByte(0x0F);
				jmpCall(op, to, instr->dest());
			}
		}

		void callOut(Output *to, Instr *instr) {
			jmpCall(true, to, instr->src());
		}

		static void shiftOp(Output *to, const Operand &dest, const Operand &src, byte subOp) {
			byte c;
			bool is8 = dest.size() == Size::sByte;

			switch (src.type()) {
			case opConstant:
				c = byte(src.constant());
				if (c == 1) {
					to->putByte(is8 ? 0xD0 : 0xD1);
					modRm(to, subOp, dest);
				} else {
					to->putByte(is8 ? 0xC0 : 0xC1);
					modRm(to, subOp, dest);
					to->putByte(c);
				}
				break;
			case opRegister:
				assert(src.reg() == cl, L"Transform of shift-op failed.");
				to->putByte(is8 ? 0xD2 : 0xD3);
				modRm(to, subOp, dest);
				break;
			default:
				assert(false, L"The transformation was not run.");
				break;
			}
		}

		void shlOut(Output *to, Instr *instr) {
			shiftOp(to, instr->dest(), instr->src(), 4);
		}

		void shrOut(Output *to, Instr *instr) {
			shiftOp(to, instr->dest(), instr->src(), 5);
		}

		void sarOut(Output *to, Instr *instr) {
			shiftOp(to, instr->dest(), instr->src(), 7);
		}

		void icastOut(Output *to, Instr *instr) {
			nat sFrom = instr->src().size().size32();
			nat sTo = instr->dest().size().size32();
			Engine &e = instr->engine();

			assert(same(instr->dest().reg(), ptrA), L"Only rax, eax, or al supported as a target for icast.");
			bool srcEax = instr->src().type() == opRegister && same(instr->src().reg(), ptrA);

			if (sFrom == 1 && sTo == 4) {
				// movsx
				to->putByte(0x0F);
				to->putByte(0xBE);
				modRm(to, registerId(eax), instr->src());
			} else if (sFrom == 4 && sTo == 8) {
				// mov (if needed).
				if (!srcEax)
					movOut(to, mov(e, eax, instr->src()));
				// cdq
				to->putByte(0x99);
			} else if (sFrom == 8 && sTo == 4) {
				if (!srcEax)
					movOut(to, mov(e, eax, low32(instr->src())));
			} else if (sFrom ==4 && sTo == 1) {
				if (!srcEax)
					movOut(to, mov(e, eax, instr->src()));
			} else if (sFrom == 8 && sTo == 8) {
				if (!srcEax) {
					movOut(to, mov(e, eax, low32(instr->src())));
					movOut(to, mov(e, edx, high32(instr->src())));
				}
			} else if (sFrom == sTo) {
				if (!srcEax) {
					Size sz = instr->src().size();
					movOut(to, mov(e, asSize(eax, sz) , instr->src()));
				}
			} else {
				assert(false, L"Unsupported icast mode: " + ::toS(instr));
			}
		}

		void ucastOut(Output *to, Instr *instr) {
			nat sFrom = instr->src().size().size32();
			nat sTo = instr->dest().size().size32();
			Engine &e = instr->engine();

			assert(same(instr->dest().reg(), ptrA), L"Only rax, eax, or al supported as a target for icast.");
			bool srcEax = instr->src().type() == opRegister && same(instr->src().reg(), ptrA);

			if (sFrom == 1 && sTo == 4) {
				// movzx
				to->putByte(0x0F);
				to->putByte(0xB6);
				modRm(to, registerId(eax), instr->src());
			} else if (sFrom == 4 && sTo == 8) {
				// mov (if needed).
				if (!srcEax)
					movOut(to, mov(e, eax, instr->src()));
				// xor edx, edx
				to->putByte(0x33);
				to->putByte(0xD2);
			} else if (sFrom == 8 && sTo == 4) {
				if (!srcEax)
					movOut(to, mov(e, eax, low32(instr->src())));
			} else if (sFrom ==4 && sTo == 1) {
				if (!srcEax)
					movOut(to, mov(e, eax, instr->src()));
			} else if (sFrom == 8 && sTo == 8) {
				if (!srcEax) {
					movOut(to, mov(e, eax, low32(instr->src())));
					movOut(to, mov(e, edx, high32(instr->src())));
				}
			} else if (sFrom == sTo) {
				if (!srcEax) {
					Size sz = instr->src().size();
					movOut(to, mov(e, asSize(eax, sz) , instr->src()));
				}
			} else {
				assert(false, L"Unsupported icast mode: " + ::toS(instr));
			}
		}

		void mulOut(Output *to, Instr *instr) {
			assert(instr->dest().type() == opRegister);
			const Operand &src = instr->src();
			Reg reg = instr->dest().reg();

			switch (src.type()) {
			case opConstant:
				if (singleByte(src.constant())) {
					to->putByte(0x6B);
					modRm(to, registerId(reg), instr->dest());
					to->putByte(src.constant() & 0xFF);
				} else {
					to->putByte(0x69);
					modRm(to, registerId(reg), instr->dest());
					to->putInt(Nat(src.constant()));
				}
				break;
			case opLabel:
			case opReference:
			case opObjReference:
				assert(false, L"Multiplying an absolute address does not make sense.");
				break;
			default:
				// Register or in memory, handled by the modRm variant.
				to->putByte(0x0F);
				to->putByte(0xAF);
				modRm(to, registerId(reg), src);
				break;
			}
		}

		void idivOut(Output *to, Instr *instr) {
			assert(instr->dest().type() == opRegister);
			assert(same(ptrA, instr->dest().reg()));
			if (instr->size() == Size::sByte) {
				to->putByte(0x66); to->putByte(0x98); // (size override) CBW
				to->putByte(0xF6); // DIV
			} else {
				to->putByte(0x99); // CDQ
				to->putByte(0xF7); // DIV
			}
			modRm(to, 7, instr->src());
		}

		void udivOut(Output *to, Instr *instr) {
			assert(instr->dest().type() == opRegister);
			assert(same(ptrA, instr->dest().reg()));
			if (instr->size() == Size::sByte) {
				to->putByte(0x30); to->putByte(0xE4); // XOR AH, AH
				to->putByte(0xF6); // DIV
			} else {
				to->putByte(0x31); to->putByte(0xD2); // XOR EDX, EDX
				to->putByte(0xF7); // DIV
			}
			modRm(to, 6, instr->src());
		}

		void leaOut(Output *to, Instr *instr) {
			Operand src = instr->src();
			Operand dest = instr->dest();
			assert(dest.type() == opRegister);
			nat regId = registerId(dest.reg());

			if (src.type() == opReference) {
				// Special meaning, load the RefSource instead.
				to->putByte(0xB8 + regId);
				to->putObject(src.refSource());
			} else {
				to->putByte(0x8D);
				modRm(to, regId, src);
			}
		}

		void fstpOut(Output *to, Instr *instr) {
			if (instr->dest().size() == Size::sDouble)
				to->putByte(0xDD);
			else
				to->putByte(0xD9);
			modRm(to, 3, instr->dest());
		}

		void fldOut(Output *to, Instr *instr) {
			if (instr->src().size() == Size::sDouble)
				to->putByte(0xDD);
			else
				to->putByte(0xD9);
			modRm(to, 0, instr->src());
		}

		static void fpOp(Output *to, Instr *instr, byte op) {
			if (instr->size() == Size::sDouble)
				to->putByte(0xF2);
			else
				to->putByte(0xF3);
			to->putByte(0x0F);
			to->putByte(op);
			modRm(to, fpRegisterId(instr->dest().reg()), instr->src());
		}

		void faddOut(Output *to, Instr *instr) {
			fpOp(to, instr, 0x58);
		}

		void fsubOut(Output *to, Instr *instr) {
			fpOp(to, instr, 0x5C);
		}

		void fmulOut(Output *to, Instr *instr) {
			fpOp(to, instr, 0x59);
		}

		void fdivOut(Output *to, Instr *instr) {
			fpOp(to, instr, 0x5E);
		}

		void fnegOut(Output *to, Instr *instr) {
			// set dest to zero, it has to be a register (using XORPS/XORPD)
			Operand dest = instr->dest();
			if (dest.size() == Size::sDouble)
				to->putByte(0x66);
			to->putByte(0x0F);
			to->putByte(0x57);
			modRm(to, fpRegisterId(dest.reg()), dest);

			// fsub
			fpOp(to, instr, 0x5C);
		}

		void fcmpOut(Output *to, Instr *instr) {
			// Note: this is COMISS/COMISD, not CMPSS/CMPSD
			if (instr->size() == Size::sDouble)
				to->putByte(0x66);
			to->putByte(0x0F);
			to->putByte(0x2F);
			modRm(to, fpRegisterId(instr->dest().reg()), instr->src());
		}

		void fcastOut(Output *to, Instr *instr) {
			Operand src = instr->src();
			Operand dst = instr->dest();

			if (src.size() == dst.size()) {
				// Just a mov operation!
				movOut(to, instr);
				return;
			}

			if (src.size() == Size::sDouble)
				to->putByte(0xF2);
			else
				to->putByte(0xF3);
			to->putByte(0x0F);
			to->putByte(0x5A);
			modRm(to, fpRegisterId(instr->dest().reg()), instr->src());
		}

		void fcastiOut(Output *to, Instr *instr) {
			Operand src = instr->src();
			Operand dst = instr->dest();
			if (dst.size() == Size::sLong) {
				// Use "old-style" FP instructions. We know that both operands are in memory, so we
				// can simply emit fld and fisttp:
				fldOut(to, instr);
				to->putByte(0xDD);
				modRm(to, 1, instr->dest());
			} else {
				// Use CVTTSS2SI or CVTTSD2SI.
				if (src.size() == Size::sDouble)
					to->putByte(0xF2);
				else
					to->putByte(0xF3);
				to->putByte(0x0F);
				to->putByte(0x2C);
				modRm(to, registerId(instr->dest().reg()), instr->src());
			}
		}

		void fcastuOut(Output *to, Instr *instr) {
			// Operand src = instr->src();
			Operand dst = instr->dest();

			// Use "old-style" FP instructions. We know that both operands are in memory, so we
			// can simply emit fld and fisttp:
			fldOut(to, instr);

			if (dst.size() == Size::sLong) {
				// Complex case. Conversions to integers are 'staurated', and we use that. We spill
				// the number to memory first, load it back and subtract from the original
				// number. That gives us a difference that we can add to the original number.
				// We know that we have 16 bytes available on the stack to do our work, and a register
				// <dsth>, <dstl> to use as a temporary.

				Reg dstl = low32(dst.reg());
				Reg dsth = high32(dst.reg());
				Operand stackPos = longRel(ptrStack, Offset());
				Operand stackLow = low32(stackPos);
				Operand stackHigh = high32(stackPos);


				// We emit the following code: (source is already loaded)
				// mov <dstl>, 0xFFFFFFFF
				// mov <stack>, <dstl>
				// shr <dstl>, 1
				// mov <stack+4>, <dstl>
				// fild <stack>
				// fcomi ST1
				// ja LARGE
				// fldz ST1
				// fcomip ST1
				// ja NORMAL
				// xor <dsth>, <dsth>
				// xor <dstl>, <dstl>
				// j DONE
				// LARGE:
				// fsubp ST1 (maybe fsubr)
				// fsttp <stack>
				// mov <dstl>, <stack>
				// mov <dsth>, <stack+4>
				// add <dstl>, 0xFFFFFFFF
				// adc <dsth>, 0x7FFFFFFF
				// j DONE
				// NORMAL:
				// fsttp <stack>
				// mov <dstl>, <stack>
				// mov <dsth>, <stack+4>
				// DONE:

				// mov <dstl>, 0xFFFFFF
				movOut(to, dstl, natConst(0xFFFFFFFF));
				// mov <stack>, <dstl>
				movOut(to, stackLow, dstl);
				// shr <dstl>
				to->putByte(0xC1);
				modRm(to, 5, dstl);
				to->putByte(0x01);
				// mov <stack+4>, <dstl>
				movOut(to, stackHigh, dstl);
				// fild
				to->putByte(0xDF);
				modRm(to, 5, stackPos);
				// fcomi ST1
				to->putByte(0xDB);
				to->putByte(0xF1);
				// ja LARGE
				to->putByte(0x0F);
				to->putByte(0x82);
				to->putInt(23);
				// fstp ST0 (effectively only a pop)
				to->putByte(0xDD);
				to->putByte(0xD8 + 0);
				// fldz ST1
				to->putByte(0xD9);
				to->putByte(0xEE);
				// fcomip ST1
				to->putByte(0xDF);
				to->putByte(0xF1);
				// ja NORMAL
				to->putByte(0x0F);
				to->putByte(0x82);
				to->putInt(40);
				// fstp ST0 (effectively only a pop)
				to->putByte(0xDD);
				to->putByte(0xD8 + 0);
				// xor x2 (4 bytes)
				bxorOut(to, dstl, dstl);
				bxorOut(to, dsth, dsth);
				// j DONE
				to->putByte(0xE9);
				to->putInt(0x27);
				// LARGE:
				// fsubp/fsubr ST0
				to->putByte(0xDE);
				to->putByte(0xE9);
				// fsttp <stack>
				to->putByte(0xDD);
				modRm(to, 1, stackPos);
				// mov x2
				movOut(to, dstl, stackLow);
				movOut(to, dsth, stackHigh);
				// add <dstl>, <stack>
				addOut(to, dstl, natConst(0xFFFFFFFF));
				// adc <dsth>, <stack+4>
				adcOut(to, dsth, natConst(0x7FFFFFFF));
				// j DONE
				to->putByte(0xE9);
				to->putInt(0xA);
				// NORMAL:
				// fsttp <stack>
				to->putByte(0xDD);
				modRm(to, 1, stackPos);
				// mov x2
				movOut(to, dstl, stackLow);
				movOut(to, dsth, stackHigh);
				// DONE:

			} else {
				// Simple case, just tell the hardware to emit a 64-bit output and return the lower
				// 32-bits of it.
				to->putByte(0xDD);
				modRm(to, 1, instr->dest());
			}
		}

		void icastfOut(Output *to, Instr *instr) {
			Operand src = instr->src();
			// Operand dst = instr->dest();
			if (src.size() == Size::sLong) {
				// Use "old-style" FP instructions. We know that both operands are in memory, so we
				// can simply emit FILD followed by FSTP
				to->putByte(0xDF);
				modRm(to, 5, src);
				fstpOut(to, instr);
			} else {
				// TODO: Need to clear target register first? (use PXOR, 66 0f ef XX)
				Operand dst = instr->dest();

				// Use CVTSI2SS or CVTSI2SD
				if (dst.size() == Size::sDouble)
					to->putByte(0xF2);
				else
					to->putByte(0xF3);
				to->putByte(0x0F);
				to->putByte(0x2A);
				modRm(to, fpRegisterId(dst.reg()), src);
			}
		}

		void ucastfOut(Output *to, Instr *instr) {
			Operand src = instr->src();
			Operand dst = instr->dest();
			if (src.size() == Size::sLong) {
				// This case is a bit tricky. We need to check the sign bit manually and adjust.
				// The idea here is to load the number as usual. If it was treated as negative, we
				// can simply compensate by adding a large enough number to it.

				// We emit the following code:
				// fild <src>
				// mov <tmp>, <src+4>
				// test <tmp>, <tmp>
				// jns DONE
				// xor <tmp>, <tmp>
				// mov <dst>, <tmp>
				// mov <tmp>, #exp
				// mov <dst+4>, <tmp>
				// fild <tmp>
				// faddp ST1
				// DONE
				// fstp

				// Here, #exp is the number 2^62 as encoded for a double:
				Nat exp = (64 + 1023) << 20;

				Reg tmp = asSize(dst.reg(), Size::sInt);
				Operand stackPos = longRel(ptrStack, Offset());
				Operand stackLow = low32(stackPos);
				Operand stackHigh = high32(stackPos);

				// fild <src>
				to->putByte(0xDF);
				modRm(to, 5, instr->src());
				// mov <tmp>, <src+4>
				movOut(to, tmp, stackHigh);
				// test <tmp>, <tmp>
				to->putByte(0x85);
				modRm(to, registerId(tmp), tmp);
				// jns DONE
				to->putByte(0x0F);
				to->putByte(0x89);
				to->putInt(20);
				// xor <tmp>, <tmp>
				bxorOut(to, tmp, tmp);
				// mov <dst>, <tmp>
				movOut(to, stackLow, tmp);
				// mov <tmp>, #exp
				movOut(to, tmp, natConst(exp));
				// mov <dst+4>, <tmp>
				movOut(to, stackHigh, tmp);
				// fild <tmp>
				to->putByte(0xDD);
				modRm(to, 0, stackPos);
				// faddp ST0
				to->putByte(0xDE);
				to->putByte(0xC1);
				// fstp <dest>
				if (dst.size() == Size::sDouble)
					to->putByte(0xDD);
				else
					to->putByte(0xD9);
				modRm(to, 3, stackPos);
			} else {
				// Issue a 64-bit load. Pre-processing ensures that this is fine to do.
				to->putByte(0xDF);
				modRm(to, 5, instr->src());
				fstpOut(to, instr);
			}
		}

		void threadLocalOut(Output *to, Instr *instr) {
			to->putByte(0x64); // FS segment
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
				assert(false, L"Unsupported type for dat.");
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
			// We don't currently offer the ability to lookup source locations in the final binary.
		}

		void metaOut(Output *, Instr *) {
			// We don't output metadata.
		}

#define OUTPUT(x) { op::x, &x ## Out }

		typedef void (*OutputFn)(Output *to, Instr *instr);

		const OpEntry<OutputFn> outputMap[] = {
			OUTPUT(epilog),
			OUTPUT(mov),
			OUTPUT(swap),
			OUTPUT(add),
			OUTPUT(adc),
			OUTPUT(bor),
			OUTPUT(band),
			OUTPUT(bnot),
			OUTPUT(sub),
			OUTPUT(sbb),
			OUTPUT(bxor),
			OUTPUT(cmp),
			OUTPUT(test),
			OUTPUT(push),
			OUTPUT(pop),
			OUTPUT(pushFlags),
			OUTPUT(popFlags),
			OUTPUT(setCond),
			OUTPUT(jmp),
			OUTPUT(call),
			OUTPUT(ret),
			OUTPUT(shl),
			OUTPUT(shr),
			OUTPUT(sar),
			OUTPUT(icast),
			OUTPUT(ucast),
			OUTPUT(mul),
			OUTPUT(idiv),
			OUTPUT(udiv),
			OUTPUT(lea),

			OUTPUT(fstp),
			OUTPUT(fld),
			OUTPUT(fadd),
			OUTPUT(fsub),
			OUTPUT(fmul),
			OUTPUT(fdiv),
			OUTPUT(fneg),
			OUTPUT(fcmp),
			OUTPUT(fcast),
			OUTPUT(fcasti),
			OUTPUT(fcastu),
			OUTPUT(icastf),
			OUTPUT(ucastf),
			// Basic testing shows that:
			// use movups instead of movaps for unaligned loads
			// unaligned operations for add/sub/movss/etc is fine

			OUTPUT(threadLocal),
			OUTPUT(dat),
			OUTPUT(lblOffset),
			OUTPUT(align),
			OUTPUT(location),
			OUTPUT(meta),
		};

		void output(Listing *src, Output *to) {
			static OpTable<OutputFn> t(outputMap, ARRAY_COUNT(outputMap));

			for (Nat i = 0; i < src->count(); i++) {
				to->mark(src->labels(i));

				Instr *instr = src->at(i);
				OutputFn fn = t[instr->op()];
				if (fn) {
					(*fn)(to, instr);
				} else {
					assert(false, L"Unsupported op-code: " + String(name(instr->op())));
				}
			}

			to->mark(src->labels(src->count()));
		}

	}
}
