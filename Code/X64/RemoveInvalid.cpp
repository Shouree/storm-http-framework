#include "stdafx.h"
#include "RemoveInvalid.h"
#include "Asm.h"
#include "Params.h"
#include "../Listing.h"
#include "../UsedRegs.h"
#include "../Exception.h"

namespace code {
	namespace x64 {

#define TRANSFORM(x) { op::x, &RemoveInvalid::x ## Tfm }
#define IMM_REG(x) { op::x, &RemoveInvalid::immRegTfm }
#define DEST_W_REG(x) { op::x, &RemoveInvalid::destRegWTfm }
#define DEST_RW_REG(x) { op::x, &RemoveInvalid::destRegRwTfm }
#define FP_OP(x) { op::x, &RemoveInvalid::fpInstrTfm }

		const OpEntry<RemoveInvalid::TransformFn> RemoveInvalid::transformMap[] = {
			IMM_REG(mov),
			IMM_REG(add),
			IMM_REG(adc),
			IMM_REG(bor),
			IMM_REG(band),
			IMM_REG(sub),
			IMM_REG(sbb),
			IMM_REG(bxor),
			IMM_REG(cmp),

			DEST_W_REG(lea),
			DEST_W_REG(icast),
			DEST_W_REG(ucast),
			DEST_RW_REG(mul),

			TRANSFORM(prolog),
			TRANSFORM(beginBlock),
			TRANSFORM(endBlock),

			TRANSFORM(fnCall),
			TRANSFORM(fnCallRef),
			TRANSFORM(fnParam),
			TRANSFORM(fnParamRef),
			// fnRet and fnRetRef are handled in the Layout transform.

			TRANSFORM(idiv),
			TRANSFORM(udiv),
			TRANSFORM(imod),
			TRANSFORM(umod),
			TRANSFORM(shr),
			TRANSFORM(sar),
			TRANSFORM(shl),

			FP_OP(fadd),
			FP_OP(fsub),
			TRANSFORM(fneg),
			FP_OP(fmul),
			FP_OP(fdiv),
			FP_OP(fcmp),
			FP_OP(fcast),
			TRANSFORM(fcasti),
			TRANSFORM(fcastu),
			TRANSFORM(icastf),
			TRANSFORM(ucastf),
		};

		static bool isComplexParam(Listing *l, Var v) {
			TypeDesc *t = l->paramDesc(v);
			if (!t)
				return false;

			return as<ComplexDesc>(t) != null;
		}

		static bool isComplexParam(Listing *l, Operand op) {
			if (op.type() != opVariable)
				return false;

			return isComplexParam(l, op.var());
		}

		RemoveInvalid::RemoveInvalid() {}

		void RemoveInvalid::before(Listing *dest, Listing *src) {
			used = usedRegs(dest->arena, src).used;
			large = new (this) Array<Operand>();
			lblLarge = dest->label();
			params = new (this) Array<ParamInfo>();

			// Set the 'freeIndirect' flag on all complex parameters.
			Array<Var> *vars = dest->allParams();
			for (Nat i = 0; i < vars->count(); i++) {
				Var v = vars->at(i);
				if (isComplexParam(dest, v)) {
					FreeOpt flags = dest->freeOpt(v);
					// Complex parameters are freed by the caller.
					flags &= ~freeOnException;
					flags &= ~freeOnBlockExit;
					flags |= freeIndirection;
					dest->freeOpt(v, flags);
				}
			}
		}

		void RemoveInvalid::during(Listing *dest, Listing *src, Nat line) {
			static OpTable<TransformFn> t(transformMap, ARRAY_COUNT(transformMap));

			Instr *i = src->at(line);
			switch (i->op()) {
			case op::call:
			case op::fnCall:
			case op::fnCallRef:
			case op::jmp:
				// Nothing needed. We deal with these later on in the chain.
				break;
			default:
				i = extractNumbers(i);
				i = extractComplex(dest, i, line);
				break;
			}

			TransformFn f = t[i->op()];
			if (f) {
				(this->*f)(dest, i, line);
			} else {
				*dest << i;
			}
		}

		void RemoveInvalid::after(Listing *dest, Listing *src) {
			// Output all constants.
			if (large->count())
				*dest << alignAs(Size::sPtr);
			*dest << lblLarge;
			for (Nat i = 0; i < large->count(); i++) {
				*dest << dat(large->at(i));
			}
		}

		Instr *RemoveInvalid::extractNumbers(Instr *i) {
			Operand src = i->src();
			if (src.type() == opConstant && src.size() == Size::sWord && !singleInt(src.constant())) {
				i = i->alterSrc(longRel(lblLarge, Offset::sWord*large->count()));
				large->push(src);
			}

			// Labels are also constants.
			if (src.type() == opLabel) {
				i = i->alterSrc(ptrRel(lblLarge, Offset::sWord*large->count()));
				large->push(src);
			}

			// Since writing to a constant is not allowed, we will not attempt to extract 'dest'.
			return i;
		}

		Instr *RemoveInvalid::extractComplex(Listing *to, Instr *i, Nat line) {
			// Complex parameters are passed as a pointer. Dereference these by inserting a 'mov' instruction.
			RegSet *regs = used->at(line);
			if (isComplexParam(to, i->src())) {
				const Operand &src = i->src();
				Reg reg = asSize(unusedReg(regs), Size::sPtr);
				regs = new (this) RegSet(*regs);
				regs->put(reg);

				*to << mov(reg, ptrRel(src.var(), Offset()));
				i = i->alterSrc(xRel(src.size(), reg, src.offset()));
			}

			if (isComplexParam(to, i->dest())) {
				const Operand &dest = i->dest();
				Reg reg = asSize(unusedReg(regs), Size::sPtr);
				*to << mov(reg, ptrRel(dest.var(), Offset()));
				i = i->alterDest(xRel(dest.size(), reg, dest.offset()));
			}

			return i;
		}

		// Is this src+dest combination supported for immReg op-codes?
		static bool supported(Instr *instr) {
			// Basically: one operand has to be a register, except when 'src' is a constant.
			switch (instr->src().type()) {
			case opConstant:
				// If a constant remains this far, it is small enough to be an immediate value!
			case opRegister:
				return true;
			default:
				if (instr->dest().type() == opRegister)
					return true;
				break;
			}

			return false;
		}

		void RemoveInvalid::immRegTfm(Listing *dest, Instr *instr, Nat line) {
			if (supported(instr)) {
				*dest << instr;
				return;
			}

			Size size = instr->src().size();
			Reg reg = unusedReg(used->at(line));
			reg = asSize(reg, size);
			*dest << mov(reg, instr->src());
			*dest << instr->alterSrc(reg);
		}

		void RemoveInvalid::destRegWTfm(Listing *dest, Instr *instr, Nat line) {
			if (instr->dest().type() == opRegister) {
				*dest << instr;
				return;
			}

			Reg reg = unusedReg(used->at(line));
			reg = asSize(reg, instr->dest().size());
			*dest << instr->alterDest(reg);
			*dest << mov(instr->dest(), reg);
		}

		void RemoveInvalid::destRegRwTfm(Listing *dest, Instr *instr, Nat line) {
			if (instr->dest().type() == opRegister) {
				*dest << instr;
				return;
			}

			Reg reg = unusedReg(used->at(line));
			reg = asSize(reg, instr->dest().size());
			*dest << mov(reg, instr->dest());
			*dest << instr->alterDest(reg);
			*dest << mov(instr->dest(), reg);
		}

		void RemoveInvalid::prologTfm(Listing *dest, Instr *instr, Nat line) {
			currentBlock = dest->root();
			*dest << instr;
		}

		void RemoveInvalid::beginBlockTfm(Listing *dest, Instr *instr, Nat line) {
			currentBlock = instr->src().block();

			// We need to tell the next step what register(s) are free.
			Reg r = asSize(unusedReg(used->at(line)), Size::sLong);
			*dest << instr->alterDest(r);
		}

		void RemoveInvalid::endBlockTfm(Listing *dest, Instr *instr, Nat line) {
			Block ended = instr->src().block();
			currentBlock = dest->parent(ended);
			*dest << instr;
		}

		void RemoveInvalid::fnParamTfm(Listing *dest, Instr *instr, Nat line) {
			TypeInstr *i = as<TypeInstr>(instr);
			if (!i) {
				throw new (this) InvalidValue(S("Expected a TypeInstr for 'fnParam'."));
			}

			params->push(ParamInfo(i->type, i->src(), false));
		}

		void RemoveInvalid::fnParamRefTfm(Listing *dest, Instr *instr, Nat line) {
			TypeInstr *i = as<TypeInstr>(instr);
			if (!i) {
				throw new (this) InvalidValue(S("Expected a TypeInstr for 'fnParamRef'."));
			}

			params->push(ParamInfo(i->type, i->src(), true));
		}

		void RemoveInvalid::fnCallTfm(Listing *dest, Instr *instr, Nat line) {
			TypeInstr *t = as<TypeInstr>(instr);
			if (!t) {
				throw new (this) InvalidValue(S("Using a fnCall that was not created properly."));
			}

			emitFnCall(dest, t->src(), t->dest(), t->type, false, currentBlock, used->at(line), params);
			params->clear();
		}

		void RemoveInvalid::fnCallRefTfm(Listing *dest, Instr *instr, Nat line) {
			TypeInstr *t = as<TypeInstr>(instr);
			if (!t) {
				throw new (this) InvalidValue(S("Using a fnCall that was not created properly."));
			}

			emitFnCall(dest, t->src(), t->dest(), t->type, true, currentBlock, used->at(line), params);
			params->clear();
		}

		void RemoveInvalid::shlTfm(Listing *dest, Instr *instr, Nat line) {
			switch (instr->src().type()) {
			case opRegister:
				if (instr->src().reg() == cl) {
					*dest << instr;
					return;
				}
				break;
			case opConstant:
				*dest << instr;
				return;
			}

			Size size = instr->dest().size();

			// We need to store the value in 'cl'. See if 'dest' is also 'cl' or 'rcx'.
			if (instr->dest().type() == opRegister && same(instr->dest().reg(), rcx)) {
				// Yes. We need to swap things around quite a lot!
				Reg reg = asSize(unusedReg(used->at(line)), size);
				*dest << mov(reg, instr->dest());
				*dest << mov(cl, instr->src());
				*dest << instr->alter(reg, cl);
				*dest << mov(instr->dest(), reg);
			} else {
				// We do not need to do that at least!
				Reg reg = asSize(unusedReg(used->at(line)), Size::sLong);
				*dest << mov(reg, rcx);
				*dest << mov(cl, instr->src());
				*dest << instr->alterSrc(cl);
				*dest << mov(rcx, reg);
			}
		}

		void RemoveInvalid::shrTfm(Listing *dest, Instr *instr, Nat line) {
			shlTfm(dest, instr, line);
		}

		void RemoveInvalid::sarTfm(Listing *dest, Instr *instr, Nat line) {
			shlTfm(dest, instr, line);
		}

		void RemoveInvalid::idivTfm(Listing *dest, Instr *instr, Nat line) {
			RegSet *used = new (this) RegSet(*this->used->at(line));
			const Operand &op = instr->dest();
			bool small = op.size() == Size::sByte;

			// If 'src' is a constant, we need to move it into a register.
			if (instr->src().type() == opConstant) {
				Reg r = asSize(unusedReg(used), instr->src().size());
				*dest << mov(r, instr->src());
				instr = instr->alterSrc(r);
			}

			// Make sure ptrD can be trashed (not needed if we're working with bytes).
			Reg oldD = noReg;
			if (!small && used->has(ptrD)) {
				oldD = asSize(unusedReg(used), Size::sPtr);
				*dest << mov(oldD, ptrD);
				used->put(oldD);
			}

			if (op.type() == opRegister && same(op.reg(), ptrA)) {
				// Supported!
				*dest << instr;
			} else {
				// We need to put op into 'ptrA'.
				Reg oldA = noReg;
				if (used->has(ptrA)) {
					oldA = asSize(unusedReg(used), Size::sPtr);
					*dest << mov(oldA, ptrA);
					used->put(oldA);
				}

				Reg destA = asSize(ptrA, op.size());
				*dest << mov(destA, op);
				if (instr->src().type() == opRegister && same(instr->src().reg(), ptrA)) {
					*dest << instr->alter(destA, asSize(oldA, instr->src().size()));
				} else {
					*dest << instr->alterDest(destA);
				}
				*dest << mov(op, destA);

				if (oldA != noReg) {
					*dest << mov(ptrA, oldA);
				}
			}

			if (oldD != noReg) {
				*dest << mov(ptrD, oldD);
			}
		}

		void RemoveInvalid::udivTfm(Listing *dest, Instr *instr, Nat line) {
			idivTfm(dest, instr, line);
		}

		void RemoveInvalid::imodTfm(Listing *dest, Instr *instr, Nat line) {
			RegSet *used = new (this) RegSet(*this->used->at(line));
			const Operand &op = instr->dest();
			bool small = op.size() == Size::sByte;

			// If 'src' is a constant, we need to move it into a register.
			if (instr->src().type() == opConstant) {
				Reg r = asSize(unusedReg(used), instr->src().size());
				*dest << mov(r, instr->src());
				instr = instr->alterSrc(r);
			}

			// Make sure ptrD can be trashed (unless we're working with 8 bit numbers).
			Reg oldD = noReg;
			if (!small && used->has(ptrD)) {
				oldD = asSize(unusedReg(used), Size::sPtr);
				*dest << mov(oldD, ptrD);
				used->put(oldD);
			}

			// We need to put op into 'ptrA'.
			Reg oldA = noReg;
			if (used->has(ptrA)) {
				oldA = asSize(unusedReg(used), Size::sPtr);
				*dest << mov(oldA, ptrA);
				used->put(oldA);
			}

			Reg destA = asSize(ptrA, op.size());
			if (op.type() != opRegister || op.reg() != destA)
				*dest << mov(destA, op);

			if (instr->src().type() == opRegister && same(instr->src().reg(), ptrA)) {
				*dest << instr->alter(destA, asSize(oldA, instr->src().size()));
			} else {
				*dest << instr->alterDest(destA);
			}

			Reg destD = asSize(ptrD, op.size());
			if (small) {
				// We need to shift the register a bit (we are not able to access AH in this implementation).
				*dest << shr(eax, byteConst(8));
				destD = al;
			}

			if (op.type() != opRegister || op.reg() != destD)
				*dest << mov(op, destD);

			// Restore registers.
			if (oldA != noReg) {
				*dest << mov(ptrA, oldA);
			}
			if (oldD != noReg) {
				*dest << mov(ptrD, oldD);
			}
		}

		void RemoveInvalid::umodTfm(Listing *dest, Instr *instr, Nat line) {
			imodTfm(dest, instr, line);
		}

		Reg RemoveInvalid::loadFpRegister(Listing *dest, const Operand &op, Nat line) {
			// Must be in a fp register!
			if (fpRegister(op))
				return op.reg();

			// Just load it into a free vector register!
			Reg r = asSize(unusedFpReg(used->at(line)), op.size());
			used->at(line)->put(r);
			*dest << mov(r, op);
			return r;
		}

		Operand RemoveInvalid::loadFpRegisterOrMemory(Listing *dest, const Operand &op, Nat line) {
			switch (op.type()) {
			case opRelative:
			case opVariable:
				return op;
			default:
				return loadFpRegister(dest, op, line);
			}
		}

		void RemoveInvalid::fpInstrTfm(Listing *dest, Instr *instr, Nat line) {
			// The XMM instructions we use support a source in memory, but not a destination.
			Operand dst = instr->dest();
			DestMode mode = destMode(instr->op());

			Reg dstReg = noReg;
			if (mode & destRead) {
				dstReg = loadFpRegister(dest, dst, line);
			} else {
				// Just pick a register if the specified one is not good enough.
				if (fpRegister(dst)) {
					dstReg = dst.reg();
				} else {
					dstReg = asSize(unusedFpReg(used->at(line)), dst.size());
					// We don't need to update the register in the used set either, usage will not overlap.
					// No need to load it, it is not read.
				}
			}

			Operand src = loadFpRegisterOrMemory(dest, instr->src(), line);

			*dest << instr->alter(dstReg, src);

			// Write it back if necessary.
			if (mode & destWrite) {
				if (dst.type() != opRegister || dst.reg() != dstReg)
					*dest << mov(dst, dstReg);
			}
		}

		void RemoveInvalid::fnegTfm(Listing *dest, Instr *instr, Nat line) {
			Operand src = loadFpRegisterOrMemory(dest, instr->src(), line);
			Operand dst = instr->dest();

			// Just pick a register if the specified one is not good enough.
			if (!fpRegister(dst)) {
				Reg dstReg = asSize(unusedFpReg(used->at(line)), dst.size());
				*dest << instr->alter(dstReg, src);
				*dest << mov(dst, dstReg);
			} else {
				*dest << instr->alterSrc(src);
			}
		}

		void RemoveInvalid::fcastiTfm(Listing *dest, Instr *instr, Nat line) {
			Operand src = loadFpRegisterOrMemory(dest, instr->src(), line);

			Operand dst = instr->dest();
			if (dst.type() != opRegister) {
				Reg r = asSize(unusedReg(used->at(line)), dst.size());
				*dest << instr->alter(r, src);
				*dest << mov(dst, r);
			} else {
				*dest << instr->alterSrc(src);
			}
		}

		void RemoveInvalid::fcastuTfm(Listing *dest, Instr *instr, Nat line) {
			Operand src = instr->src();
			Operand dst = instr->dest();

			src = loadFpRegisterOrMemory(dest, src, line);

			if (dst.size() == Size::sDouble) {
				// This is a bit tricky. We need to examine the number and adjust it if it is too large.
				Reg tmpReg = noReg;
				if (dst.type() == opRegister)
					tmpReg = dst.reg();
				if (tmpReg == noReg)
					tmpReg = asSize(unusedReg(used->at(line)), Size::sLong);
				used->at(line)->put(tmpReg);
				Reg tmpReg2 = asSize(unusedReg(used->at(line)), Size::sLong);
				Reg fpReg = asSize(unusedFpReg(used->at(line)), Size::sDouble);
				used->at(line)->put(fpReg);

				Reg srcCopy = asSize(unusedFpReg(used->at(line)), Size::sDouble);

				Label done = dest->label();
				Label normal = dest->label();
				Label zero = dest->label();

				// Copy source to a double.
				if (src.size() != Size::sDouble)
					*dest << fcast(srcCopy, src);
				else
					*dest << mov(srcCopy, src);

				// Check if below zero:
				*dest << bxor(tmpReg, tmpReg);
				*dest << ucastf(fpReg, tmpReg);
				*dest << fcmp(fpReg, srcCopy);
				*dest << jmp(zero, ifFAboveEqual);

				// Create 0x7FFF...FFF
				*dest << mov(tmpReg, wordConst(1));
				*dest << shl(tmpReg, byteConst(63));
				*dest << sub(tmpReg, wordConst(1));

				// Convert it to a floating point number.
				*dest << icastf(fpReg, tmpReg);

				// If the value is larger than the cutoff, then we need to adjust it.
				*dest << fcmp(fpReg, srcCopy);
				*dest << jmp(normal, ifFAbove);

				// It is large. Subtract, extract, and then add back the difference.
				*dest << fsub(srcCopy, fpReg);
				*dest << fcastu(tmpReg2, srcCopy);
				*dest << add(tmpReg, tmpReg2);
				*dest << jmp(done);

				*dest << zero;
				*dest << bxor(tmpReg, tmpReg);
				*dest << jmp(done);

				// Normal. Just extract it.
				*dest << normal;
				*dest << fcastu(tmpReg, srcCopy);

				// Store back.
				*dest << done;

				if (dst.type() != opRegister || dst.reg() != tmpReg) {
					*dest << mov(dst, tmpReg);
				}

			} else {
				src = loadFpRegisterOrMemory(dest, src, line);

				// We can simply write to the output register as if it was 64-bit in size.
				if (dst.type() != opRegister) {
					Reg r = asSize(unusedReg(used->at(line)), Size::sLong);
					*dest << instr->alter(r, src);
					*dest << ucast(dst, r);
				} else {
					Reg largeDst = asSize(dst.reg(), Size::sLong);
					*dest << instr->alter(largeDst, src);
					*dest << ucast(dst.reg(), largeDst);
				}
			}
		}

		void RemoveInvalid::icastfTfm(Listing *dest, Instr *instr, Nat line) {
			Operand dst = instr->dest();
			if (fpRegister(dst)) {
				*dest << instr;
			} else {
				Reg r = asSize(unusedFpReg(used->at(line)), dst.size());
				*dest << instr->alterDest(r);
				*dest << mov(dst, r);
			}
		}

		void RemoveInvalid::ucastfTfm(Listing *dest, Instr *instr, Nat line) {
			Operand src = instr->src();
			Operand dst = instr->dest();

			if (src.size() == Size::sLong) {
				// This is a bit tricky. We need to check the sign bit manually for this to work.
				Reg tmpReg = asSize(unusedReg(used->at(line)), Size::sLong);
				// work with doubles to avoid rounding errors during conversion
				Reg fpReg = noReg;
				if (fpRegister(dst))
					fpReg = asSize(dst.reg(), Size::sDouble);
				else
					fpReg = asSize(unusedFpReg(used->at(line)), Size::sDouble);

				Label done = dest->label();

				// Floating-point value that we shall add if the number is negative.
				// MSVC wants extra paren to not emit warning...
				Word fpAdd = (Word(64 + 1023)) << 52;

				*dest << mov(tmpReg, src);
				*dest << ucastf(fpReg, src);
				*dest << shl(tmpReg, byteConst(1)); // behaves same as cmp to zero in this case
				// *dest << cmp(tmpReg, longConst(0));
				*dest << jmp(done, ifAboveEqual);
				*dest << fadd(fpReg, longRel(lblLarge, Offset::sWord * large->count()));
				large->push(wordConst(fpAdd));

				*dest << done;
				if (!fpRegister(dst) || fpReg != dst.reg()) {
					if (dst.size() != size(fpReg)) {
						// Cast to float if necessary. Output of fcast has to be a register.
						*dest << fcast(asSize(fpReg, dst.size()), fpReg);
						fpReg = asSize(fpReg, dst.size());
					}
					*dest << mov(dst, fpReg);
				}

			} else {
				// We can simply cast the source to a longer size, and use that.
				Reg longSrc = asSize(unusedReg(used->at(line)), Size::sLong);
				*dest << ucast(longSrc, src);

				if (fpRegister(dst)) {
					*dest << instr->alterSrc(longSrc);
				} else {
					Reg r = asSize(unusedFpReg(used->at(line)), dst.size());
					*dest << instr->alter(r, longSrc);
					*dest << mov(dst, r);
				}
			}
		}

	}
}
