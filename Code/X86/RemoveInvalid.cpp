#include "stdafx.h"
#include "RemoveInvalid.h"
#include "Listing.h"
#include "Exception.h"
#include "Asm.h"
#include "Utils/Bitwise.h"

namespace code {
	namespace x86 {

#define IMM_REG(x) { op::x, &RemoveInvalid::immRegTfm }
#define TRANSFORM(x) { op::x, &RemoveInvalid::x ## Tfm }
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

			TRANSFORM(beginBlock),

			TRANSFORM(lea),
			TRANSFORM(mul),
			TRANSFORM(idiv),
			TRANSFORM(udiv),
			TRANSFORM(imod),
			TRANSFORM(umod),
			TRANSFORM(setCond),
			TRANSFORM(shl),
			TRANSFORM(shr),
			TRANSFORM(sar),
			TRANSFORM(icast),
			TRANSFORM(ucast),

			TRANSFORM(fnParam),
			TRANSFORM(fnParamRef),
			TRANSFORM(fnCall),
			TRANSFORM(fnCallRef),

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

		RemoveInvalid::Param::Param(Operand src, TypeDesc *type, Bool ref) : src(src), type(type), ref(ref) {}

		RemoveInvalid::RemoveInvalid() {}

		void RemoveInvalid::before(Listing *dest, Listing *src) {
			params = new (this) Array<Param>();

			used = usedRegs(dest->arena, src).used;

			// Add 64-bit aliases everywhere.
			for (nat i = 0; i < used->count(); i++)
				add64(used->at(i));
		}

		void RemoveInvalid::during(Listing *dest, Listing *src, Nat line) {
			static OpTable<TransformFn> t(transformMap, ARRAY_COUNT(transformMap));

			Instr *i = src->at(line);
			TransformFn f = t[i->op()];
			if (f) {
				(this->*f)(dest, i, line);
			} else {
				*dest << i;
			}
		}

		Reg RemoveInvalid::unusedReg(Nat line) {
			return code::x86::unusedReg(used->at(line));
		}

		// ImmReg combination already supported?
		static bool supported(Instr *instr) {
			switch (instr->src().type()) {
			case opLabel:
			case opReference:
			case opConstant:
			case opObjReference:
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
			assert(size.size32() <= Size::sInt.size32(), "The 64-bit transform should have fixed this!");

			Reg reg = unusedReg(line);
			if (reg == noReg) {
				reg = asSize(ptrD, size);
				*dest << push(ptrD);
				*dest << mov(reg, instr->src());
				*dest << instr->alterSrc(reg);
				*dest << pop(ptrD);
			} else {
				reg = asSize(reg, size);
				*dest << mov(reg, instr->src());
				*dest << instr->alterSrc(reg);
			}
		}

		void RemoveInvalid::beginBlockTfm(Listing *dest, Instr *instr, Nat line) {
			// We need to tell the next step what register(s) are free.
			Reg r = unusedReg(line);
			if (r != noReg)
				*dest << instr->alterDest(r);
		}

		void RemoveInvalid::leaTfm(Listing *dest, Instr *instr, Nat line) {

			// We can encode writing directly to a register.
			if (instr->dest().type() == opRegister) {
				*dest << instr;
				return;
			}

			Reg reg = unusedReg(line);
			if (reg == noReg) {
				*dest << push(ptrD);
				*dest << lea(ptrD, instr->src());
				*dest << mov(instr->dest(), ptrD);
				*dest << pop(ptrD);
			} else {
				reg = asSize(reg, Size::sPtr);
				*dest << lea(reg, instr->src());
				*dest << mov(instr->dest(), reg);
			}
		}

		void RemoveInvalid::mulTfm(Listing *dest, Instr *instr, Nat line) {

			Size size = instr->size();
			assert(size.size32() <= Size::sInt.size32(), "Bytes not supported yet!");

			if (instr->dest().type() == opRegister) {
				*dest << instr;
				return;
			}

			// Only supported mmode is mul <reg>, <r/m>. Move dest into a register.
			Reg reg = unusedReg(line);
			if (reg == noReg) {
				reg = asSize(ptrD, size);
				*dest << push(ptrD);
				*dest << mov(reg, instr->dest());
				*dest << instr->alterDest(reg);
				*dest << mov(instr->dest(), reg);
				*dest << pop(ptrD);
			} else {
				reg = asSize(reg, size);
				*dest << mov(reg, instr->dest());
				*dest << instr->alterDest(reg);
				*dest << mov(instr->dest(), reg);
			}
		}

		void RemoveInvalid::idivTfm(Listing *to, Instr *instr, Nat line) {
			Operand dest = instr->dest();
			bool srcConst = instr->src().type() == opConstant;
			bool destEax = false;

			if (dest.type() == opRegister && same(dest.reg(), ptrA)) {
				destEax = true;

				if (!srcConst) {
					// Supported!
					*to << instr;
					return;
				}
			}

			// The 64-bit transform has been executed before, so we are sure that size is <= sInt
			bool isByte = dest.size() == Size::sByte;
			Operand newSrc = instr->src();

			RegSet *used = this->used->at(line);

			// Clear eax and edx.
			if (!destEax && used->has(eax))
				*to << push(eax);
			if (!isByte && used->has(edx))
				*to << push(edx);

			// TODO: if 'src' is 'eax', then we're screwed.

			// Move dest into eax first.
			if (isByte)
				*to << bxor(eax, eax);
			*to << mov(asSize(eax, dest.size()), dest);

			if (srcConst) {
				if (used->has(ebx))
					*to << push(ebx);
				*to << mov(ebx, instr->src());
				newSrc = ebx;
			}

			// Note: we do not need to clear edx here, AsmOut will do that for us, ie. we treat edx
			// as an output-only register.
			*to << instr->alter(asSize(eax, dest.size()), newSrc);
			*to << mov(dest, asSize(eax, dest.size()));

			if (srcConst && used->has(ebx))
				*to << pop(ebx);
			if (!isByte && used->has(edx))
				*to << pop(edx);
			if (!destEax && used->has(eax))
				*to << pop(eax);
		}

		void RemoveInvalid::udivTfm(Listing *dest, Instr *instr, Nat line) {
			idivTfm(dest, instr, line);
		}

		void RemoveInvalid::imodTfm(Listing *to, Instr *instr, Nat line) {
			Operand dest = instr->dest();
			bool srcConst = instr->src().type() == opConstant;
			bool eaxDest = dest.type() == opRegister && same(dest.reg(), ptrA);
			bool isByte = dest.size() == Size::sByte;

			Operand newSrc = instr->src();
			RegSet *used = this->used->at(line);

			// Clear eax and edx if needed.
			if (!eaxDest && used->has(eax))
				*to << push(eax);
			if (!isByte && used->has(edx))
				*to << push(edx);

			// TODO: if 'src' is 'eax', then we're screwed.

			// Move dest into eax first.
			if (isByte)
				*to << bxor(eax, eax);
			*to << mov(asSize(eax, dest.size()), dest);

			if (srcConst) {
				if (used->has(ebx))
					*to << push(ebx);
				*to << mov(ebx, instr->src());
				newSrc = ebx;
			}

			// Note: we do not need to clear edx here, AsmOut will do that for us, ie. we treat edx
			// as an output-only register.
			if (instr->op() == op::imod)
				*to << idiv(asSize(eax, dest.size()), newSrc);
			else
				*to << udiv(asSize(eax, dest.size()), newSrc);

			if (isByte) {
				*to << shr(eax, byteConst(8));
				*to << mov(dest, asSize(eax, dest.size()));
			} else {
				*to << mov(dest, edx);
			}


			if (srcConst && used->has(ebx))
				*to << pop(ebx);
			if (!isByte && used->has(edx))
				*to << pop(edx);
			if (!eaxDest && used->has(eax))
				*to << pop(eax);
		}

		void RemoveInvalid::umodTfm(Listing *dest, Instr *instr, Nat line) {
			imodTfm(dest, instr, line);
		}

		void RemoveInvalid::setCondTfm(Listing *dest, Instr *instr, Nat line) {

			switch (instr->src().condFlag()) {
			case ifAlways:
				*dest << mov(engine(), instr->dest(), byteConst(1));
				break;
			case ifNever:
				*dest << mov(engine(), instr->dest(), byteConst(0));
				break;
			default:
				*dest << instr;
				break;
			}
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
				// Supported!
				*dest << instr;
				return;
			}

			Size size = instr->dest().size();

			// We need to store the value in cl. See if dest is also cl or ecx:
			if (instr->dest().type() == opRegister && same(instr->dest().reg(), ecx)) {
				// Yup. We need to swap things around a lot!
				Reg reg = asSize(unusedReg(line), size);

				if (reg == noReg) {
					// Ugh... Worst case!
					*dest << push(ecx);
					*dest << mov(cl, instr->src());
					*dest << instr->alter(xRel(size, ptrStack, Offset(0)), cl);
					*dest << pop(ecx);
				} else {
					*dest << mov(reg, instr->dest());
					*dest << mov(cl, instr->src());
					*dest << instr->alter(reg, cl);
					*dest << mov(instr->dest(), reg);
				}
			} else {
				// We have a bit more leeway at least!
				Reg reg = asSize(unusedReg(line), Size::sInt);

				if (reg == noReg) {
					*dest << push(ecx);
					*dest << mov(cl, instr->src());
					*dest << instr->alterSrc(cl);
					*dest << pop(ecx);
				} else {
					*dest << mov(reg, ecx);
					*dest << mov(cl, instr->src());
					*dest << instr->alterSrc(cl);
					*dest << mov(ecx, reg);
				}
			}
		}

		void RemoveInvalid::shrTfm(Listing *dest, Instr *instr, Nat line) {
			shlTfm(dest, instr, line);
		}

		void RemoveInvalid::sarTfm(Listing *dest, Instr *instr, Nat line) {
			shlTfm(dest, instr, line);
		}

		void RemoveInvalid::icastTfm(Listing *dest, Instr *instr, Nat line) {
			Operand to = instr->dest();
			Size sFrom = instr->src().size();
			Size sTo = to.size();

			if (instr->dest() == Operand(asSize(eax, sTo))) {
				*dest << instr;
				return;
			}

			bool toEax = to.type() == opRegister && same(to.reg(), eax);
			bool toEaxRel = to.type() == opRelative && same(to.reg(), eax);

			RegSet *used = this->used->at(line);
			bool saveEax = used->has(eax);
			bool saveEdx = used->has(edx);
			bool saveEcx = used->has(ecx);

			if (toEax)
				saveEax = false;
			if (sFrom != Size::sLong && sTo != Size::sLong)
				saveEdx = false;
			if (!toEaxRel)
				saveEcx = false;

			if (saveEdx)
				*dest << push(edx);
			if (saveEcx)
				*dest << push(ecx);
			if (saveEax)
				*dest << push(eax);

			if ((sFrom == Size::sByte && sTo == Size::sLong) ||
				(sFrom == Size::sLong && sTo == Size::sByte)) {
				*dest << instr->alterDest(eax);
				*dest << instr->alter(asSize(eax, sTo), eax);
			} else {
				*dest << instr->alterDest(asSize(eax, sTo));
			}

			if (!toEax) {
				if (toEaxRel) {
					// Read the old eax...
					*dest << mov(ptrC, ptrRel(ptrStack, Offset()));
					to = xRel(to.size(), ptrC, to.offset());
				}

				if (sTo == Size::sLong) {
					*dest << mov(low32(to), eax);
					*dest << mov(high32(to), edx);
				} else {
					*dest << mov(to, asSize(eax, sTo));
				}
			}

			if (saveEax)
				*dest << pop(eax);
			if (saveEcx)
				*dest << pop(ecx);
			if (saveEdx)
				*dest << pop(edx);
		}

		void RemoveInvalid::ucastTfm(Listing *dest, Instr *instr, Nat line) {
			icastTfm(dest, instr, line);
		}

		void RemoveInvalid::fnParamTfm(Listing *dest, Instr *instr, Nat line) {
			TypeInstr *ti = as<TypeInstr>(instr);
			if (!ti) {
				throw new (this) InvalidValue(S("Expected a TypeInstr for 'fnParam'."));
			}

			params->push(Param(ti->src(), ti->type, false));
		}

		void RemoveInvalid::fnParamRefTfm(Listing *dest, Instr *instr, Nat line) {
			TypeInstr *ti = as<TypeInstr>(instr);
			if (!ti) {
				throw new (this) InvalidValue(S("Expected a TypeInstr for 'fnParamRef'."));
			}

			params->push(Param(ti->src(), ti->type, true));
		}

		static Operand offset(const Operand &src, Offset offset) {
			switch (src.type()) {
			case opVariable:
				return xRel(Size::sInt, src.var(), offset);
			case opRegister:
				return xRel(Size::sInt, src.reg(), offset);
			default:
				assert(false, L"Can not generate offsets into this type!");
				return Operand();
			}
		}

		static void pushMemcpy(Listing *dest, const Operand &src) {
			if (src.size().size32() <= Size::sInt.size32()) {
				*dest << push(src);
				return;
			}

			Nat size = roundUp(src.size().size32(), Nat(4));
			for (nat i = 0; i < size; i += 4) {
				*dest << push(offset(src, Offset(size - i - 4)));
			}
		}

		static void inlinedMemcpy(Listing *to, const Operand &src, Offset offset, Size sz) {
			Nat size = roundUp(sz.size32(), Nat(4));
			// All registers used here are destroyed during function calls.
			if (src.type() != opRegister || !same(src.reg(), ptrA))
				*to << mov(ptrA, src);
			for (nat i = 0; i < size; i += 4) {
				*to << mov(edx, intRel(ptrA, Offset(i)));
				*to << mov(intRel(ptrStack, Offset(i) + offset), edx);
			}
		}

		// Subtract 'offset' from ptrStack if required. Resets it to 0 afterwards. Used to implement
		// 'lazy' subtraction of ptrStack in 'fnCall' below.
		static void subStack(Listing *dest, Offset &offset) {
			if (offset != Offset())
				*dest << sub(ptrStack, ptrConst(offset));
			offset = Offset();
		}

		void RemoveInvalid::fnCall(Listing *dest, TypeInstr *instr, Array<Param> *params) {
			assert(instr->src().type() != opRegister, L"Not supported.");

			// Returning a reference?
			Bool retRef = instr->op() == op::fnCallRef;

			// Do we need a parameter for the return value?
			if (resultParam(instr->type)) {
				Nat id = instr->member ? 1 : 0;
				// In case there are no parameters, the result parameter becomes the only one. This is not
				// really compliant with the calling convention, but there is nothing else we can do!
				id = min(id, params->count());
				params->insert(id, Param(instr->dest(), null, false));
			} else if (retRef) {
				// Perhaps we need to store the result on the stack?
				if (instr->dest().type() == opRegister)
					*dest << push(instr->dest());
			}

			// Push all parameters we can right now. For references and things that need a copy
			// constructor, store the address on the stack for now and get back to them later.
			Offset delayedSub;
			for (Nat i = params->count(); i > 0; i--) {
				Param &p = params->at(i - 1);
				bool first = i == 1;

				if (!p.type) {
					subStack(dest, delayedSub);

					if (retRef) {
						*dest << push(instr->dest());
					} else {
						// We need an additional register for this. Do it later!
						*dest << push(ptrConst(0));
					}
				} else if (as<ComplexDesc>(p.type) == null && !p.ref) {
					subStack(dest, delayedSub);

					// Push it to the stack now!
					pushMemcpy(dest, p.src);
				} else {
					// Copy the parameter later.
					Size s = p.type->size();
					s += Size::sPtr.alignment();

					// Note: Not needed for the first parameter. We do not have time to clobber
					// registers until we apply the first parameter, so we might as well just push
					// it straight away.
					if (!first && p.ref && p.src.hasRegister()) {
						// Include everything but the last 4 bytes in the 'sub' operation. We use a
						// 'push' for those since 'push src' is able to handle more addressing modes
						// compared to 'mov [ptrBase + 0x??], src'
						delayedSub += s;
						delayedSub -= Offset::sPtr;
						subStack(dest, delayedSub);

						// Store the source of the reference here for later. We might clobber this
						// register during the next phase!
						*dest << push(p.src);
					} else {
						// Remember that we shall adjust esp, so that multiple parameters can be
						// condensed into a single sub instruction.
						delayedSub += s;
						// *dest << sub(ptrStack, ptrConst(s));
					}
				}
			}

			subStack(dest, delayedSub);

			// Now, we can use any registers we like!
			// Note: If 'retRef' is false and we require a parameter for the return value, we know
			// that the return value reside in memory somewhere, otherwise we can not use 'lea' with it!

			// Cumulated offset from esp.
			Offset paramOffset;

			for (Nat i = 0; i < params->count(); i++) {
				Param &p = params->at(i);
				bool first = i == 0;

				Size s = p.type ? p.type->size() : Size::sPtr;
				s += Size::sPtr.alignment();

				if (!p.type) {
					if (!retRef) {
						*dest << lea(ptrA, p.src);
						*dest << mov(ptrRel(ptrStack, paramOffset), ptrA);
					}
				} else if (ComplexDesc *c = as<ComplexDesc>(p.type)) {
					if (!first && p.ref && p.src.hasRegister()) {
						// If the pointer was pushed on the stack, use that as we might have clobbered a used register.
						*dest << push(ptrRel(ptrStack, paramOffset));
					} else if (p.ref) {
						*dest << push(p.src);
					} else {
						*dest << lea(ptrA, p.src);
						*dest << push(ptrA);
					}

					*dest << lea(ptrA, ptrRel(ptrStack, paramOffset + Offset::sPtr));
					*dest << push(ptrA);
					*dest << call(c->ctor, Size());
					*dest << add(ptrStack, ptrConst(Size::sPtr * 2));
				} else if (p.ref) {
					// Copy it using an inlined memcpy.
					if (!first && p.src.hasRegister()) {
						*dest << mov(ptrA, ptrRel(ptrStack, paramOffset));
						inlinedMemcpy(dest, ptrA, paramOffset, p.type->size());
					} else {
						inlinedMemcpy(dest, p.src, paramOffset, p.type->size());
					}
				}

				paramOffset += s;
			}

			// Call the function! (We do not need to analyze register usage anymore, this is fine).
			*dest << call(instr->src(), Size());

			// Pop the stack.
			if (paramOffset != Offset())
				*dest << add(ptrStack, ptrConst(paramOffset));

			// Handle the return value if needed.
			if (PrimitiveDesc *p = as<PrimitiveDesc>(instr->type)) {
				Operand to = instr->dest();

				if (retRef) {
					if (to.type() == opRegister) {
						// Previously stored on the stack, restore it!
						*dest << pop(ptrC);
					} else {
						*dest << mov(ptrC, to);
					}
					to = xRel(p->size(), ptrC, Offset());
				}

				switch (p->v.kind()) {
				case primitive::none:
					break;
				case primitive::integer:
				case primitive::pointer:
					if (to.type() == opRegister && same(to.reg(), ptrA)) {
						// Nothing to do!
					} else if (to.size() == Size::sLong) {
						*dest << mov(high32(to), edx);
						*dest << mov(low32(to), eax);
					} else {
						*dest << mov(to, asSize(ptrA, to.size()));
					}
					break;
				case primitive::real:
					if (to.type() == opRegister) {
						*dest << sub(ptrStack, ptrConst(to.size()));
						*dest << fstp(xRel(to.size(), ptrStack, Offset()));
						if (to.size() == Size::sDouble) {
							*dest << pop(low32(to));
							*dest << pop(high32(to));
						} else {
							*dest << pop(to);
						}
					} else {
						*dest << fstp(to);
					}
					break;
				}
			}
		}

		void RemoveInvalid::fnCallTfm(Listing *dest, Instr *instr, Nat line) {
			// Idea: Scan backwards to find fnCall op-codes rather than saving them in an
			// array. This could catch stray fnParam op-codes if done right. We could also do it the
			// other way around, letting fnParam search for a terminating fnCall and be done there.

			TypeInstr *t = as<TypeInstr>(instr);
			if (!t) {
				throw new (this) InvalidValue(S("Expected a TypeInstr for 'fnCall'."));
			}

			fnCall(dest, t, params);

			params->clear();
		}

		void RemoveInvalid::fnCallRefTfm(Listing *dest, Instr *instr, Nat line) {
			// Idea: Scan backwards to find fnCall op-codes rather than saving them in an
			// array. This could catch stray fnParam op-codes if done right. We could also do it the
			// other way around, letting fnParam search for a terminating fnCall and be done there.

			TypeInstr *t = as<TypeInstr>(instr);
			if (!t) {
				throw new (this) InvalidValue(S("Expected a TypeInstr for 'fnCallRef'."));
			}

			fnCall(dest, t, params);

			params->clear();
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
				if (dst.type() != opRegister || dst.reg() != dstReg) {
					*dest << mov(dst, dstReg);
				}
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
			// If the output size is 32-bit we use SSE instructions directly. If 64-bit, it is
			// easier to fall back to the "old" FP unit as it has an operation for integer
			// conversions directly.
			Operand dst = instr->dest();
			if (dst.size() == Size::sLong) {
				// x87 FP stack. Source and destination need to be in memory.
				Operand src = instr->src();
				bool spillToStack = dst.type() == opRegister || src.type() == opRegister;

				if (spillToStack) {
					*dest << sub(ptrStack, ptrConst(Size::sLong));
				}

				if (src.type() == opRegister) {
					Operand stackTmp = xRel(src.size(), ptrStack, Offset());
					*dest << mov(stackTmp, src);
					src = stackTmp;
				}

				if (dst.type() == opRegister) {
					*dest << instr->alter(xRel(dst.size(), ptrStack, Offset()), src);
				} else {
					*dest << instr->alterSrc(src);
				}

				if (spillToStack) {
					if (dst.type() == opRegister) {
						*dest << pop(low32(dst));
						*dest << pop(high32(dst));
					} else {
						*dest << add(ptrStack, ptrConst(Size::sLong));
					}
				}

			} else {
				// SSE. Need source in fp register or memory. Destination has to be integer register.
				Operand src = loadFpRegisterOrMemory(dest, instr->src(), line);
				if (dst.type() != opRegister) {
					Reg r = asSize(unusedReg(line), dst.size());
					*dest << instr->alter(r, src);
					*dest << mov(dst, r);
				} else {
					*dest << instr->alterSrc(src);
				}
			}
		}

		void RemoveInvalid::fcastuTfm(Listing *dest, Instr *instr, Nat line) {
			// In case we are asked to convert to a 32-bit uint, we always use the x87 FP stack.
			// For the 32-bit output case, we simply convert to a 64-bit int and return the lower
			// 32 bits. For the 64-bit case, we need to emit a bit more elaborate code that needs
			// a register and 64-bits of temporary storage on the stack.

			Operand src = instr->src();
			Operand dst = instr->dest();
			if (dst.size() == Size::sLong) {
				Size stackSize = Size::sLong;

				// A temporary register is needed as destination.
				Reg dstReg = noReg;
				if (dst.type() == opRegister) {
					dstReg = dst.reg();
				}
				if (dstReg == noReg) {
					const Reg options[3] = { rax, rbx, rcx };
					RegSet *used = this->used->at(line);
					for (Nat i = 0; i < ARRAY_COUNT(options); i++) {
						if (!used->has(low32(options[i])) && !used->has(high32(options[i]))) {
							dstReg = options[i];
							break;
						}
					}
				}
				Bool saveRax = false;
				if (dstReg == noReg) {
					dstReg = rax;
					saveRax = true;
					stackSize += Size::sLong;
				}

				*dest << sub(ptrStack, ptrConst(stackSize));

				if (saveRax) {
					*dest << mov(intRel(ptrStack, Offset::sLong), eax);
					*dest << mov(intRel(ptrStack, Offset::sLong + Offset::sInt), edx);
				}

				// Source needs to be in memory.
				if (src.type() == opRegister) {
					Operand stackTmp = xRel(src.size(), ptrStack, Offset());
					*dest << mov(stackTmp, src);
					src = stackTmp;
				}

				*dest << instr->alter(dstReg, src);

				if (saveRax) {
					*dest << mov(eax, intRel(ptrStack, Offset::sLong));
					*dest << mov(edx, intRel(ptrStack, Offset::sLong + Offset::sInt));
				}

				// Copy result to proper location.
				if (dst.type() != opRegister || dst.reg() != dstReg) {
					*dest << mov(low32(dst), low32(dstReg));
					*dest << mov(high32(dst), high32(dstReg));
				}
				*dest << add(ptrStack, ptrConst(stackSize));
			} else {
				// We always need to spill to the stack since the output will be longer than the target.
				*dest << sub(ptrStack, ptrConst(Size::sLong));

				if (src.type() == opRegister) {
					Operand stackTmp = xRel(src.size(), ptrStack, Offset());
					*dest << mov(stackTmp, src);
					src = stackTmp;
				}

				*dest << instr->alter(intRel(ptrStack, Offset()), src);

				if (dst.type() == opRegister) {
					*dest << mov(dst, intRel(ptrStack, Offset()));
				} else {
					Reg tmpReg = unusedReg(line);
					if (tmpReg == noReg) {
						*dest << push(eax);
						*dest << mov(eax, intRel(ptrStack, Offset()));
						*dest << mov(dst, eax);
						*dest << pop(eax);
					} else {
						tmpReg = asSize(tmpReg, Size::sInt);
						*dest << mov(tmpReg, intRel(ptrStack, Offset()));
						*dest << mov(dst, tmpReg);
					}
				}

				*dest << add(ptrStack, ptrConst(Size::sLong));
			}
		}

		void RemoveInvalid::icastfTfm(Listing *dest, Instr *instr, Nat line) {
			// If the output size is 32-bit we use SSE instructions directly. If 64-bit, it is
			// easier to fall back to the "old" FP unit as it has an operation for integer
			// conversions directly.
			Operand src = instr->src();
			if (src.size() == Size::sLong) {
				// x87 FP stack. Source and destination need to be in memory.
				Operand dst = instr->dest();
				bool spillToStack = dst.type() == opRegister || src.type() == opRegister;

				if (spillToStack) {
					if (src.type() == opRegister) {
						*dest << push(high32(src));
						*dest << push(low32(src));
					} else {
						*dest << sub(ptrStack, ptrConst(Size::sLong));
					}
				}

				if (dst.type() == opRegister) {
					Operand stackTmp = xRel(dst.size(), ptrStack, Offset());
					*dest << instr->alter(stackTmp, src);
					*dest << mov(dst, stackTmp);
				} else {
					*dest << instr->alterSrc(src);
				}

				if (spillToStack) {
					*dest << add(ptrStack, ptrConst(Size::sLong));
				}

			} else {
				// SSE. Need source in register or memory. Destination has to be integer register.
				Operand dst = instr->dest();
				if (fpRegister(dst)) {
					*dest << instr;
				} else {
					Reg r = asSize(unusedFpReg(used->at(line)), dst.size());
					*dest << instr->alterDest(r);
					*dest << mov(dst, r);
				}
			}
		}

		void RemoveInvalid::ucastfTfm(Listing *dest, Instr *instr, Nat line) {
			// For unsigned values, we always use the x87 stack. For Nat, we simply spill it to the
			// stack and load a 64-bit integer value. For Word, we need slightly more complex
			// machine code. That code requires the source to be in a register, the destination in
			// memory, and 8 bytes of stack space allocated for it.

			Operand src = instr->src();
			Operand dst = instr->dest();
			Size stackSize = Size::sLong;

			// We need a temporary register. If dest is a register, we can use that.
			Reg tmpReg = noReg;
			if (dst.type() == opRegister)
				tmpReg = dst.reg();
			if (tmpReg == noReg)
				tmpReg = unusedReg(line);
			Bool saveEax = false;
			if (tmpReg == noReg) {
				tmpReg = eax;
				saveEax = true;
				stackSize += Size::sLong;
			}

			tmpReg = asSize(tmpReg, Size::sInt);

			*dest << sub(ptrStack, ptrConst(stackSize));

			if (saveEax)
				*dest << mov(intRel(ptrStack, Offset::sLong), eax);

			Bool copyResult = false;


			if (src.size() == Size::sLong) {
				// Source needs to be in memory.
				if (src.type() == opRegister) {
					*dest << mov(intRel(ptrStack, Offset()), low32(src));
					*dest << mov(intRel(ptrStack, Offset::sInt), high32(src));
				} else {
					*dest << mov(tmpReg, low32(src));
					*dest << mov(intRel(ptrStack, Offset()), tmpReg);
					*dest << mov(tmpReg, high32(src));
					*dest << mov(intRel(ptrStack, Offset::sInt), tmpReg);
				}

				// Inform the next step where the result is located.
				*dest << instr->alter(asSize(tmpReg, dst.size()), longRel(ptrStack, Offset()));
				copyResult = true;

			} else {
				// We always need to copy to the stack to ensure that we have space for zero
				// extension.

				if (src.type() == opRegister) {
					*dest << mov(intRel(ptrStack, Offset()), src);
				} else {
					*dest << mov(tmpReg, src);
					*dest << mov(intRel(ptrStack, Offset()), tmpReg);
				}
				*dest << bxor(tmpReg, tmpReg);
				*dest << mov(intRel(ptrStack, Offset::sInt), tmpReg);

				if (dst.type() == opRegister) {
					*dest << instr->alter(xRel(dst.size(), ptrStack, Offset()), intRel(ptrStack, Offset()));
					copyResult = true;
				} else {
					*dest << instr->alterSrc(intRel(ptrStack, Offset()));
				}
			}


			if (saveEax)
				*dest << mov(eax, intRel(ptrStack, Offset::sLong));

			// Copy result back to where it belongs.
			if (dst.type() == opRegister) {
				*dest << mov(dst, xRel(dst.size(), ptrStack, Offset()));
			} else {
				Reg fpReg = asSize(unusedFpReg(used->at(line)), dst.size());
				*dest << mov(fpReg, xRel(dst.size(), ptrStack, Offset()));
				*dest << mov(dst, fpReg);
			}

			*dest << add(ptrStack, ptrConst(Size::sLong));
		}

	}
}
