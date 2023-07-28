#include "stdafx.h"
#include "AsmOut.h"
#include "Asm.h"
#include "../OpTable.h"

namespace code {
	namespace x64 {

		void prologOut(Output *to, Instr *instr) {
			// There are two forms we need to consider:
			Operand src = instr->src();

			if (src.type() == opNone) {
				// 1: if we have no parameter. This is used on Posix platforms. There, we generate a
				// prolog with DWARF metadata as follows:
				// - push rbp
				// - mov rbp, rsp

				// push rbp
				to->putByte(0x50 + 5);
				// now the CFA offset is different
				to->setFrameOffset(Offset::sPtr*2);
				// We also saved RBP on the stack
				to->markSaved(ptrFrame, -Offset::sPtr*2);

				// mov rbp, rsp
				to->putByte(0x48);
				to->putByte(0x89);
				to->putByte(0xE5);
				// now we use ebp as the CFA register, offset is the same
				to->setFrameRegister(ptrFrame);
			} else if (src.type() == opConstant) {
				// 2: if we have a constant as a parameter, we are on Windows. Then, generate code
				// for allocating memory on the stack, and emit appropriate metadata:

				// This is from 'sub' below:
				ImmRegInstr op = {
					opCode(0x83), 5,
					opCode(0x81), 5,
					opCode(0x29),
					opCode(0x2B)
				};
				immRegInstr(to, op, ptrStack, src);
				to->markFrameAlloc(src.offset());
				to->markPrologEnd();
			}
		}

		void epilogOut(Output *to, Instr *instr) {
			// Marking all epilogs require that we can resize the FDA records on demand, as there
			// might be an unknown number of return statements in a function.
			// In general, the epilog CFA information is not strictly neccessary as long as there
			// are no exceptions thrown during the last two instructions in the epilog.
			// to->markEpilog();

			// We use this pseudo-op to generate the LEAVE instruction.
			to->putByte(0xC9);
		}

		void preserveOut(Output *to, Instr *instr) {
			if (instr->dest().type() == opNone) {
				to->markSaved(instr->src().reg(), Offset());
			} else {
				// Offset between the stack pointer and the CFA. This difference is due to us
				// spilling RBP from the previous function, and the return address on the stack.
				Size offset = Size::sPtr * 2;
				to->markSaved(instr->src().reg(), instr->dest().offset() - offset);
			}
		}

		void locationOut(Output *, Instr *) {
			// We don't currently offer the ability to lookup source locations in the final binary.
		}

		void metaOut(Output *, Instr *) {
			// We don't preserve metadata.
		}

		void pushOut(Output *to, Instr *instr) {
			const Operand &src = instr->src();
			switch (src.type()) {
			case opConstant:
				assert(singleInt(src.constant()), L"Should be solved by RemoveInvalid.");
				if (singleByte(src.constant())) {
					to->putByte(0x6A);
					to->putByte(Byte(src.constant() & 0xFF));
				} else {
					to->putByte(0x68);
					to->putInt(Int(src.constant()));
				}
				break;
			case opRegister:
			{
				nat reg = registerId(src.reg());
				if (reg >= 8) {
					to->putByte(0x41); // REX.B
					reg -= 8;
				}
				to->putByte(0x50 + reg);
				break;
			}
			case opRelative:
			case opReference:
			case opObjReference:
				modRm(to, opCode(0xFF), rmNone, 6, src);
				break;
			default:
				assert(false, L"Push does not support this operand type.");
				break;
			}
		}

		void popOut(Output *to, Instr *instr) {
			const Operand &dest = instr->dest();
			switch (dest.type()) {
			case opRegister:
			{
				nat reg = registerId(dest.reg());
				if (reg >= 8) {
					to->putByte(0x41); // REX.B
					reg -= 8;
				}
				to->putByte(0x58 + reg);
				break;
			}
			case opRelative:
				modRm(to, opCode(0x8F), rmNone, 0, dest);
				break;
			default:
				assert(false, L"Pop does not support this operand type.");
				break;
			}
		}

		static OpCode fpOp(const Operand &size, byte op) {
			if (size.size() == Size::sWord)
				return prefixOpCode(0xF2, 0x0F, op);
			else
				return prefixOpCode(0xF3, 0x0F, op);
		}

		static bool stackPos(Operand &op) {
			if (op.type() != opRegister)
				return false;

			// We can use a few bytes just above the top of the stack freely, at least on UNIX.
			TODO(L"This is not ideal on Windows!");
			op = xRel(op.size(), ptrStack, -Offset(op.size()));
			return true;
		}

		void movOut(Output *to, Instr *instr) {
			// NOTE: There is a version of MOV that supports 64-bit immediate values. We could use
			// that to avoid additional instructions in quite some cases!
			ImmRegInstr8 op8 = {
				opCode(0xC6), 0,
				opCode(0x88),
				opCode(0x8A),
			};
			ImmRegInstr op = {
				opCode(0x00), 0xFF, // Not supported
				opCode(0xC7), 0,
				opCode(0x89),
				opCode(0x8B),
			};

			bool fpSrc = fpRegister(instr->src());
			bool fpDest = fpRegister(instr->dest());
			if (fpSrc && fpDest) {
				// Two registers. This is the easy part!
				modRm(to, fpOp(instr->src(), 0x10), rmNone, instr->dest(), instr->src());
			} else if (fpSrc) {
				// 'dest' is either in memory or a regular register.
				Operand dest = instr->dest();
				bool spill = stackPos(dest);
				modRm(to, fpOp(dest, 0x11), rmNone, instr->src(), dest);
				if (spill) {
					// Spilled to memory. Emit a regular 'mov' to load into the desired register.
					immRegInstr(to, op8, op, instr->dest(), dest);
				}
			} else if (fpDest) {
				// 'src' is either in memory or a regular register.
				Operand src = instr->src();
				if (stackPos(src)) {
					// We need to spill to memory. Emit a regular 'mov'.
					immRegInstr(to, op8, op, src, instr->src());
				}
				modRm(to, fpOp(src, 0x10), rmNone, instr->dest(), src);
			} else {
				// Integer mov instruction.
				immRegInstr(to, op8, op, instr->dest(), instr->src());
			}
		}

		void swapOut(Output *to, Instr *instr) {
			if (instr->size() == Size::sByte) {
				modRm(to, opCode(0x86), rmByte, instr->dest(), instr->src());
			} else {
				modRm(to, opCode(0x87), wide(instr->dest()), instr->dest(), instr->src());
			}
		}

		void addOut(Output *to, Instr *instr) {
			ImmRegInstr8 op8 = {
				opCode(0x80), 0,
				opCode(0x00),
				opCode(0x02),
			};
			ImmRegInstr op = {
				opCode(0x83), 0,
				opCode(0x81), 0,
				opCode(0x01),
				opCode(0x03)
			};
			immRegInstr(to, op8, op, instr->dest(), instr->src());
		}

		void adcOut(Output *to, Instr *instr) {
			ImmRegInstr8 op8 = {
				opCode(0x80), 2,
				opCode(0x10),
				opCode(0x12)
			};
			ImmRegInstr op = {
				opCode(0x83), 2,
				opCode(0x81), 2,
				opCode(0x11),
				opCode(0x13)
			};
			immRegInstr(to, op8, op, instr->dest(), instr->src());
		}

		void borOut(Output *to, Instr *instr) {
			ImmRegInstr8 op8 = {
				opCode(0x80), 1,
				opCode(0x08),
				opCode(0x0A)
			};
			ImmRegInstr op = {
				opCode(0x83), 1,
				opCode(0x81), 1,
				opCode(0x09),
				opCode(0x0B)
			};
			immRegInstr(to, op8, op, instr->dest(), instr->src());
		}

		void bandOut(Output *to, Instr *instr) {
			ImmRegInstr8 op8 = {
				opCode(0x80), 4,
				opCode(0x20),
				opCode(0x22)
			};
			ImmRegInstr op = {
				opCode(0x83), 4,
				opCode(0x81), 4,
				opCode(0x21),
				opCode(0x23)
			};
			immRegInstr(to, op8, op, instr->dest(), instr->src());
		}

		void bnotOut(Output *to, Instr *instr) {
			const Operand &dest = instr->dest();
			if (dest.size() == Size::sByte) {
				modRm(to, opCode(0xF6), rmNone, 2, dest);
			} else {
				modRm(to, opCode(0xF7), wide(dest), 2, dest);
			}
		}

		void subOut(Output *to, Instr *instr) {
			ImmRegInstr8 op8 = {
				opCode(0x80), 5,
				opCode(0x28),
				opCode(0x2A)
			};
			ImmRegInstr op = {
				opCode(0x83), 5,
				opCode(0x81), 5,
				opCode(0x29),
				opCode(0x2B)
			};
			immRegInstr(to, op8, op, instr->dest(), instr->src());
		}

		void sbbOut(Output *to, Instr *instr) {
			ImmRegInstr8 op8 = {
				opCode(0x80), 2,
				opCode(0x18),
				opCode(0x1A)
			};
			ImmRegInstr op = {
				opCode(0x83), 2,
				opCode(0x81), 2,
				opCode(0x19),
				opCode(0x1B)
			};
			immRegInstr(to, op8, op, instr->dest(), instr->src());
		}

		void bxorOut(Output *to, Instr *instr) {
			ImmRegInstr8 op8 = {
				opCode(0x80), 6,
				opCode(0x30),
				opCode(0x32)
			};
			ImmRegInstr op = {
				opCode(0x83), 6,
				opCode(0x81), 6,
				opCode(0x31),
				opCode(0x33)
			};
			immRegInstr(to, op8, op, instr->dest(), instr->src());
		}

		void cmpOut(Output *to, Instr *instr) {
			ImmRegInstr8 op8 = {
				opCode(0x80), 7,
				opCode(0x38),
				opCode(0x3A)
			};
			ImmRegInstr op = {
				opCode(0x83), 7,
				opCode(0x81), 7,
				opCode(0x39),
				opCode(0x3B)
			};
			immRegInstr(to, op8, op, instr->dest(), instr->src());
		}

		void testOut(Output *to, Instr *instr) {
			ImmRegInstr8 op8 = {
				opCode(0xF6), 0,
				opCode(0x84),
				opCode(0x84), // TEST is symmetric, order does not matter
			};
			ImmRegInstr op = {
				opCode(0x00), 0xFF, // not supported
				opCode(0xF7), 0,
				opCode(0x85),
				opCode(0x85), // TEST is symmetric, order does not matter
			};
			immRegInstr(to, op8, op, instr->dest(), instr->src());
		}

		static void shiftOp(Output *to, const Operand &dest, const Operand &src, byte subOp) {
			RmFlags flags = wide(dest);
			if (dest.size() == Size::sByte)
				flags |= rmByte;

			byte c;
			switch (src.type()) {
			case opConstant:
				c = byte(src.constant());
				if (c == 0) {
					// Nothing to do!
				} else if (c == 1) {
					if (flags & rmByte)
						modRm(to, opCode(0xD0), flags, subOp, dest);
					else
						modRm(to, opCode(0xD1), flags, subOp, dest);
				} else {
					if (flags & rmByte)
						modRm(to, opCode(0xC0), flags, subOp, dest);
					else
						modRm(to, opCode(0xC1), flags, subOp, dest);
					to->putByte(c);
				}
				break;
			case opRegister:
				assert(src.reg() == cl, L"Transformation of shift operation failed.");
				if (flags & rmByte)
					modRm(to, opCode(0xD2), flags, subOp, dest);
				else
					modRm(to, opCode(0xD3), flags, subOp, dest);
				break;
			default:
				assert(false, L"The shift operation was not transformed properly.");
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

		void leaOut(Output *to, Instr *instr) {
			Operand src = instr->src();
			Operand dest = instr->dest();
			assert(dest.type() == opRegister);
			nat regId = registerId(dest.reg());

			if (src.type() == opReference) {
				// Special meaning, load the RefSource instead.
				// Issues a 'mov' operation to simply load the address of the refsource.
				modRm(to, opCode(0x8B), rmWide, regId, objPtr(src.refSource()));
			} else {
				modRm(to, opCode(0x8D), rmWide, regId, src);
			}
		}

		void mulOut(Output *to, Instr *instr) {
			assert(instr->dest().type() == opRegister);
			const Operand &src = instr->src();
			Reg dest = instr->dest().reg();

			// Note: We're actually doing 32-bit multiplication even if we are only supposed to do
			// 8-bit multiplication. There is only one 8-bit multiplication which is difficult to
			// use. That is fine since any 'noise' in the registers will be lost in the
			// multiplication anyway. The only downside is when operands are in memory, which means
			// we might casue a pagefault.

			switch (src.type()) {
			case opConstant:
				if (singleByte(src.constant())) {
					modRm(to, opCode(0x6B), wide(src), registerId(dest), instr->dest());
					to->putByte(src.constant() & 0xFF);
				} else {
					modRm(to, opCode(0x69), wide(src), registerId(dest), instr->dest());
					to->putInt(Nat(src.constant()));
				}
				break;
			case opLabel:
			case opReference:
			case opObjReference:
				assert(false, L"Multiplying an absolute address does not make sense.");
				break;
			default:
				// Register or in memory. Handled by the modRm variant.
				modRm(to, opCode(0x0F, 0xAF), wide(src), registerId(dest), src);
				break;
			}
		}

		void idivOut(Output *to, Instr *instr) {
			assert(instr->dest().type() == opRegister);
			assert(same(instr->dest().reg(), ptrA));

			if (instr->size() == Size::sByte) {
				// Sign extend al into ax.
				to->putByte(0x66);
				to->putByte(0x98);
				modRm(to, opCode(0xF6), rmByte, 7, instr->src());
			} else if (instr->size() == Size::sInt) {
				// Sign-extend eax into eax, edx
				to->putByte(0x99);
				modRm(to, opCode(0xF7), rmNone, 7, instr->src());
			} else {
				// Sign extend rax into rax, rdx
				to->putByte(0x48); to->putByte(0x99);
				modRm(to, opCode(0xF7), rmWide, 7, instr->src());
			}
		}

		void udivOut(Output *to, Instr *instr) {
			assert(instr->dest().type() == opRegister);
			assert(same(instr->dest().reg(), ptrA));

			if (instr->size() == Size::sByte) {
				// Clear ah using xor ah, ah. We can't use 'modRm' for this since we can not represent the 'ah' register.
				to->putByte(0x30);
				to->putByte(0xE4);
				modRm(to, opCode(0xF6), rmByte, 6, instr->src());
			} else {
				RmFlags f = wide(instr->src());
				// Clear edx using xor edx, edx (or rdx, rdx if 64-bit)
				modRm(to, opCode(0x31), f, registerId(edx), edx);
				modRm(to, opCode(0xF7), f, 6, instr->src());
			}
		}

		void imodOut(Output *to, Instr *instr) {
			idivOut(to, instr);
		}

		void umodOut(Output *to, Instr *instr) {
			udivOut(to, instr);
		}

		static void jmpCall(Output *to, bool call, const Operand &src) {
			switch (src.type()) {
			case opConstant:
				assert(false, L"Calling or jumping to constant values are not supported. Use labels or references!");
				break;
			case opLabel:
				to->putByte(call ? 0xE8 : 0xE9);
				to->putRelative(src.label());
				break;
			case opReference:
				// We have two options of encoding here, depending on if the reference is currently
				// within the 2GB we can address using the 32 bit operand. We want to use the short
				// encoding if possible (as that is probably faster, and will occur quite frequently
				// in practice). This is handled by the implementation in 'Refs.cpp'. This
				// implementation dictates that the short variant shall be encoded with an unneeded
				// REX prefix (we set the REX.W bit, so that we pretend that we mean something).
				//
				// We start by outputting the short version of the instruction so that the 'Refs'
				// implementation is able to deduce if we're encoding a jump or a call
				// instruction. Then we let that implementation deal with the fact that we might
				// actually need the long version of the jump/call.
				to->putByte(0x48); // REX.W
				to->putByte(call ? 0xE8 : 0xE9);
				to->putGc(GcCodeRef::jump, 4, src.ref());
				break;
			case opRegister:
			case opRelative:
				modRm(to, opCode(0xFF), rmNone, call ? 2 : 4, src);
				break;
			default:
				assert(false, L"Unsupported operand used for 'call' or 'jump'.");
				break;
			}
		}

		void jmpOut(Output *to, Instr *instr) {
			CondFlag c = instr->src().condFlag();
			if (c == ifAlways) {
				jmpCall(to, false, instr->dest());
			} else if (c == ifNever) {
				// Nothing.
			} else {
				byte op = 0x80 + condOp(c);
				const Operand &src = instr->dest();
				switch (src.type()) {
				case opLabel:
					to->putByte(0x0F);
					to->putByte(op);
					to->putRelative(src.label());
					break;
				default:
					assert(false, L"Conditional jumps only support labels.");
					break;
				}
			}
		}

		void callOut(Output *to, Instr *instr) {
			jmpCall(to, true, instr->src());
		}

		void retOut(Output *to, Instr *instr) {
			to->putByte(0xC3);
		}

		void setCondOut(Output *to, Instr *instr) {
			CondFlag c = instr->src().condFlag();
			if (c == ifAlways) {
				// mov <dest>, 1
				modRm(to, opCode(0xC6), rmByte, 0, instr->dest());
				to->putByte(0x01);
			} else if (c == ifNever) {
				// mov <dest>, 0
				modRm(to, opCode(0xC6), rmByte, 0, instr->dest());
				to->putByte(0x00);
			} else {
				modRm(to, opCode(0x0F, 0x90 + condOp(c)), rmNone, 0, instr->dest());
			}
		}

		void ucastOut(Output *to, Instr *instr) {
			nat sFrom = instr->src().size().size64();
			nat sTo = instr->dest().size().size64();
			Reg rTo = instr->dest().reg();

			bool same = instr->src().type() == opRegister
				&& instr->src().reg() == rTo;

			switch (min(sTo, sFrom)) {
			case 1:
				// We could use 'movzx' as well, but this works in all cases.

				// Easy, just move one byte from 'src' to 'dest', or do an 'and' operation.
				if (!same) {
					// mov
					modRm(to, opCode(0x8A), rmByte, registerId(rTo), instr->src());
				}

				// Clear the remainder of the register, just in case.
				// and <rTo>, 0xFF
				modRm(to, opCode(0x81), rmNone, 4, asSize(rTo, Size::sInt));
				to->putInt(0xFF);
				break;
			case 4:
				// Easy, always a mov instruction. The upper 32 bits of a register is cleared when
				// accessing them as a 32-bit register. Therefore, it is acceptable to generate a
				// 'mov x, x' instruction to do this.
				modRm(to, opCode(0x8B), rmNone, registerId(rTo), instr->src());
				break;
			case 8:
				// This will rarely happen, but let's generate a regular mov instruction anyway.
				modRm(to, opCode(0x8B), rmNone, registerId(rTo), instr->src());
				break;
			default:
				assert(false, L"Unsupported operand sizes for ucast.");
				break;
			}
		}

		void icastOut(Output *to, Instr *instr) {
			nat sFrom = instr->src().size().size64();
			nat sTo = instr->dest().size().size64();
			Reg rTo = instr->dest().reg();

			if (sFrom >= sTo) {
				// We can utilize the fact that narrowing works the same way for both signed and
				// unsigned integers.
				ucastOut(to, instr);
				return;
			}

			if (sTo == 4 && sFrom == 1) {
				// movsx
				modRm(to, opCode(0x0F, 0xBE), rmByte, registerId(rTo), instr->src());
			} else if (sTo == 8 && sFrom == 1) {
				// movsx
				modRm(to, opCode(0x0F, 0xBE), rmWide | rmByte, registerId(rTo), instr->src());
			} else if (sTo == 8 && sFrom == 4) {
				// movsx
				modRm(to, opCode(0x0F, 0xBF), rmWide, registerId(rTo), instr->src());
			} else {
				assert(false, L"Unsupported operand sizes for icast.");
			}
		}

		void fstpOut(Output *to, Instr *instr) {
			if (instr->size() == Size::sDouble) {
				modRm(to, opCode(0xDD), rmNone, 3, instr->dest());
			} else {
				modRm(to, opCode(0xD9), rmNone, 3, instr->dest());
			}
		}

		void fldOut(Output *to, Instr *instr) {
			if (instr->size() == Size::sDouble) {
				modRm(to, opCode(0xDD), rmNone, 0, instr->src());
			} else {
				modRm(to, opCode(0xD9), rmNone, 0, instr->src());
			}
		}

		static void fpOp(Output *to, Instr *instr, OpCode op) {
			if (instr->size() == Size::sDouble)
				op.prefix = 0xF2;
			else
				op.prefix = 0xF3;
			modRm(to, op, rmNone, instr->dest(), instr->src());
		}

		void faddOut(Output *to, Instr *instr) {
			fpOp(to, instr, opCode(0x0F, 0x58));
		}

		void fsubOut(Output *to, Instr *instr) {
			fpOp(to, instr, opCode(0x0F, 0x5C));
		}

		void fmulOut(Output *to, Instr *instr) {
			fpOp(to, instr, opCode(0x0F, 0x59));
		}

		void fdivOut(Output *to, Instr *instr) {
			fpOp(to, instr, opCode(0x0F, 0x5E));
		}

		void fnegOut(Output *to, Instr *instr) {
			// set dest to zero, it has to be a register (using XORPS/XORPD)
			Operand dest = instr->dest();
			if (dest.size() == Size::sDouble)
				modRm(to, prefixOpCode(0x66, 0x0F, 0x57), rmNone, dest, dest);
			else
				modRm(to, opCode(0x0F, 0x57), rmNone, dest, dest);
			// fsub
			fpOp(to, instr, opCode(0x0F, 0x5C));
		}

		void fcmpOut(Output *to, Instr *instr) {
			// Note: this is COMISS/COMISD, not CMPSS/CMPSD
			OpCode op = instr->size() == Size::sDouble
				? prefixOpCode(0x66, 0x0F, 0x2F)
				: opCode(0x0F, 0x2F);
			modRm(to, op, rmNone, instr->dest(), instr->src());
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
				modRm(to, prefixOpCode(0xF2, 0x0F, 0x5A), rmNone, instr->dest(), instr->src());
			else
				modRm(to, prefixOpCode(0xF3, 0x0F, 0x5A), rmNone, instr->dest(), instr->src());
		}

		void fcastiOut(Output *to, Instr *instr) {
			byte prefix = instr->src().size() == Size::sDouble
				? 0xF2 : 0xF3;
			modRm(to, prefixOpCode(prefix, 0x0F, 0x2C), wide(instr->dest()), instr->dest(), instr->src());
		}

		void fcastuOut(Output *to, Instr *instr) {
			// TODO: Do something sensible!
			fcastiOut(to, instr);
		}

		void icastfOut(Output *to, Instr *instr) {
			// TODO: Need to clear target register first? (use PXOR, 66 0f ef XX)

			byte prefix = instr->dest().size() == Size::sDouble
				? 0xF2 : 0xF3;
			modRm(to, prefixOpCode(prefix, 0x0F, 0x2A), wide(instr->src()), instr->dest(), instr->src());
		}

		void ucastfOut(Output *to, Instr *instr) {
			// TODO: Do something sensible!
			icastfOut(to, instr);
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

		const OpEntry<OutputFn> outputMap[] = {
			OUTPUT(push),
			OUTPUT(pop),
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
			OUTPUT(shl),
			OUTPUT(shr),
			OUTPUT(sar),
			OUTPUT(lea),
			OUTPUT(mul),
			OUTPUT(idiv),
			OUTPUT(udiv),
			OUTPUT(imod),
			OUTPUT(umod),
			OUTPUT(jmp),
			OUTPUT(call),
			OUTPUT(ret),
			OUTPUT(setCond),
			OUTPUT(icast),
			OUTPUT(ucast),

			// Debug information.
			OUTPUT(prolog),
			OUTPUT(epilog),
			OUTPUT(preserve),
			OUTPUT(location),
			OUTPUT(meta),

			// Floating point.
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

			OUTPUT(dat),
			OUTPUT(lblOffset),
			OUTPUT(align),
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
