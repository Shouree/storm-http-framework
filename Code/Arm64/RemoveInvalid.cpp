#include "stdafx.h"
#include "RemoveInvalid.h"
#include "Asm.h"
#include "Utils/Bitmask.h"
#include "../Listing.h"
#include "../UsedRegs.h"
#include "../Exception.h"
#include "../Binary.h"
#include "Gc/DwarfTable.h"

namespace code {
	namespace arm64 {

		/**
		 * Table of operations to transform individual instructions.
		 */

#define TRANSFORM(x) { op::x, &RemoveInvalid::x ## Tfm }
#define DATA12(x) { op::x, &RemoveInvalid::dataInstr12Tfm }
#define BITMASK(x) { op::x, &RemoveInvalid::bitmaskInstrTfm }
#define DATA4REG(x) { op::x, &RemoveInvalid::dataInstr4RegTfm }
#define SHIFT(x) { op::x, &RemoveInvalid::shiftInstrTfm }
#define FP_OP(x) { op::x, &RemoveInvalid::fpInstrTfm }

		const OpEntry<RemoveInvalid::TransformFn> RemoveInvalid::transformMap[] = {
			TRANSFORM(prolog),
			TRANSFORM(epilog),
			TRANSFORM(beginBlock),
			TRANSFORM(endBlock),

			TRANSFORM(fnCall),
			TRANSFORM(fnCallRef),
			TRANSFORM(fnParam),
			TRANSFORM(fnParamRef),
			TRANSFORM(mov),
			TRANSFORM(lea),
			TRANSFORM(swap),
			TRANSFORM(cmp),
			TRANSFORM(setCond),
			TRANSFORM(icast),
			TRANSFORM(ucast),

			DATA12(add),
			DATA12(sub),
			DATA4REG(mul),
			TRANSFORM(idiv),
			TRANSFORM(udiv),
			TRANSFORM(umod),
			TRANSFORM(imod),

			BITMASK(band),
			BITMASK(bor),
			BITMASK(bxor),
			BITMASK(bnot),

			SHIFT(shl),
			SHIFT(shr),
			SHIFT(sar),

			FP_OP(fadd),
			FP_OP(fsub),
			FP_OP(fneg),
			FP_OP(fmul),
			FP_OP(fdiv),
			FP_OP(fcmp),

			TRANSFORM(fcast),
			TRANSFORM(fcasti),
			TRANSFORM(fcastu),
			TRANSFORM(icastf),
			TRANSFORM(ucastf),
		};


		/**
		 * Table with information on how to handle byte-sized operands for instructions.
		 *
		 * ARM only has 32- and 64-bit variants of certain operations. So we need to compensate in
		 * certain cases when working with bytes to ensure that garbage in the upper-parts of the
		 * 32-bit register will not be visible.
		 */

		enum ByteTransform {
			none = 0x00,
			extendSrc = 0x01,
			signedSrc = 0x02,
			extendDest = 0x10,
			signedDest = 0x20,
		};

		BITMASK_OPERATORS(ByteTransform);

		static const OpEntry<ByteTransform> byteTransformMap[] = {
			{ op::cmp, extendSrc | extendDest }, // Bytes are always unsigned in Storm (correct
												 // impl. would need to know sign...)
			{ op::udiv, extendSrc | extendDest },
			{ op::umod, extendSrc | extendDest },
			{ op::idiv, extendSrc | extendDest | signedSrc | signedDest },
			{ op::idiv, extendSrc | extendDest | signedSrc | signedDest },
			{ op::shr, extendDest },
			{ op::shl, extendDest | signedDest },
		};


		RemoveInvalid::RemoveInvalid() {}

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

		void RemoveInvalid::before(Listing *dest, Listing *src) {
			currentBlock = Block();
			used = usedRegs(dest->arena, src).used;
			params = new (this) Array<ParamInfo>();
			large = new (this) Array<Operand>();
			lblLarge = dest->label();

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

			Instr *original = src->at(line);

			Instr *i = modifyByte(dest, original, line);

			TransformFn f = t[i->op()];
			if (f) {
				(this->*f)(dest, i, line);
			} else {
				*dest << i;
			}

			// If the byte transform had to modify the destination, make sure to store it back where it belongs.
			if (i != original && i->dest() != original->dest()) {
				*dest << mov(original->dest(), i->dest());
			}
		}

		static Engine *findEngine() {
#ifdef ARM64
			void **fp;
			__asm__ ( "mov %0, x29"
					: "=r" (fp)
					: : );
			void *pc = fp[1];
			storm::FDE *fde = storm::dwarfTable().find(pc);
			if (!fde)
				return null;

			Binary *b = codeBinary(fde->codeStart());
			if (!b)
				return null;

			return &b->engine();
#else
			return null;
#endif
		}

		extern "C" void SHARED_EXPORT throwDivisionException() {
			Engine *e = findEngine();
			if (!e)
				e = runtime::someEngineUnsafe();
			throw new (*e) DivisionByZero();
		}

		void RemoveInvalid::after(Listing *dest, Listing *src) {
			if (lblDivZero != Label()) {
				*dest << lblDivZero;
				const void *errorFn = address(&throwDivisionException);
				*dest << mov(ptrr(16), largeConstant(xConst(Size::sPtr, (Word)errorFn)));
				*dest << call(ptrr(16), Size());
			}

			for (Nat i = 0; i < large->count(); i++) {
				*dest << alignAs(Size::sPtr);
				if (i == 0)
					*dest << lblLarge;
				*dest << dat(large->at(i));
			}
		}

		Label RemoveInvalid::divZeroLabel(Listing *dest) {
			if (lblDivZero == Label())
				lblDivZero = dest->label();
			return lblDivZero;
		}

		/**
		 * Byte operands.
		 */

		Instr *RemoveInvalid::modifyByte(Listing *to, Instr *instr, Nat line) {
			static OpTable<ByteTransform> t(byteTransformMap, ARRAY_COUNT(byteTransformMap));

			ByteTransform tfm = t[instr->op()];

			if (tfm & extendSrc) {
				Operand src = instr->src();
				if (src.size() == Size::sByte) {
					instr = instr->alterSrc(modifyByte(to, src, line, (tfm & signedSrc) != 0));
				}
			}

			if (tfm & extendDest) {
				Operand dest = instr->dest();
				if (dest.size() == Size::sByte) {
					instr = instr->alterDest(modifyByte(to, dest, line, (tfm & signedDest) != 0));
				}
			}

			return instr;
		}

		Operand RemoveInvalid::modifyByte(Listing *to, const Operand &op, Nat line, Bool opSigned) {
			if (op.type() == opRegister) {
				// If it was already in a register, we need to insert a suitable cast to a 32-bit register first.
				Reg r = op.reg();
				if (opSigned) {
					*to << icast(asSize(r, Size::sInt), r);
				} else {
					*to << ucast(asSize(r, Size::sInt), r);
				}
				return op;

			} else if (opSigned) {
				// In other cases, we only need to worry if signed operations are used. For
				// unsigned, we can simply load bytes from memory with the proper semantics. For
				// signed, we need to sign-extend.
				Reg r = unusedReg(used->at(line), op.size());
				used->at(line)->put(r);

				// Note: icast supports these addressing modes!
				*to << icast(r, op);

				return Operand(r);
			} else {
				return op;
			}
		}


		/**
		 * Helpers.
		 */

		Operand RemoveInvalid::largeConstant(const Operand &data) {
			Nat offset = large->count();
			large->push(data);
			return xRel(data.size(), lblLarge, Offset::sWord * offset);
		}

		Operand RemoveInvalid::limitImm(Listing *to, const Operand &op, Nat line, Nat immBits, Bool immSigned) {
			if (op.type() != opConstant)
				return op;

			Bool ok = false;
			if (immSigned) {
				Long v = op.constant();
				Long maxVal = Long(1) << (immBits - 1);
				ok = (v >= -maxVal) & (v < maxVal);
			} else {
				Word maxVal = Word(1) << immBits;
				ok = op.constant() < maxVal;
			}

			if (ok)
				return op;

			Reg r = unusedReg(used->at(line), op.size());
			used->at(line)->put(r);
			loadRegister(to, r, op);
			return Operand(r);
		}

		Instr *RemoveInvalid::limitImmSrc(Listing *to, Instr *instr, Nat line, Nat immBits, Bool immSigned) {
			Operand src = limitImm(to, instr->src(), line, immBits, immSigned);
			if (src != instr->src())
				return instr->alterSrc(src);
			else
				return instr;
		}

		Operand RemoveInvalid::regOrImm(Listing *to, const Operand &op, Nat line, Nat immBits, Bool immSigned) {
			if (op.type() == opRegister)
				return op;

			if (op.type() == opConstant)
				return limitImm(to, op, line, immBits, immSigned);

			// TODO: Maybe exclude other types here as well?

			// Otherwise, try to load it into a register.
			Reg r = unusedReg(used->at(line), op.size());
			used->at(line)->put(r);
			loadRegister(to, r, op);
			return Operand(r);
		}

		void RemoveInvalid::loadRegister(Listing *to, Reg reg, const Operand &load) {
			switch (load.type()) {
			case opConstant:
				if (load.constant() <= 0xFFFF && isIntReg(reg)) // No load literal for fp registers.
					*to << mov(reg, load);
				else
					*to << mov(reg, largeConstant(load));
				break;

			case opRegister:
				if (!same(reg, load.reg()))
					*to << mov(reg, load);
				break;

				// TODO: Maybe others?
			default:
				*to << mov(reg, load);
				break;
			}
		}

		void RemoveInvalid::removeMemoryRefs(Listing *to, Instr *instr, Nat line) {
			Operand src = instr->src();

			switch (src.type()) {
			case opRegister:
			case opConstant:
			case opNone:
				// TODO: More things to ignore here!
				break;
			default:
				// Emit a load first.
				Reg t = unusedReg(used->at(line), src.size());
				used->at(line)->put(t);
				loadRegister(to, t, src);
				src = t;
				break;
			}

			Operand dest = instr->dest();
			if (dest.type() != opRegister) {
				// Load and store the destination.
				Reg t = unusedReg(used->at(line), dest.size());
				loadRegister(to, t, dest);
				*to << instr->alter(t, src);
				*to << mov(dest, t);
			} else {
				*to << instr->alterSrc(src);
			}
		}


		/**
		 * Bookkeeping
		 */

		void RemoveInvalid::prologTfm(Listing *to, Instr *instr, Nat line) {
			currentBlock = to->root();
			*to << instr;
		}

		void RemoveInvalid::epilogTfm(Listing *to, Instr *instr, Nat line) {
			currentBlock = Block();
			*to << instr;
		}

		void RemoveInvalid::beginBlockTfm(Listing *to, Instr *instr, Nat line) {
			currentBlock = instr->src().block();
			*to << instr;
		}

		void RemoveInvalid::endBlockTfm(Listing *to, Instr *instr, Nat line) {
			Block ended = instr->src().block();
			currentBlock = to->parent(ended);
			*to << instr;
		}


		/**
		 * Function calls.
		 */

		void RemoveInvalid::fnParamTfm(Listing *to, Instr *instr, Nat line) {
			TypeInstr *i = as<TypeInstr>(instr);
			if (!i) {
				throw new (this) InvalidValue(S("Expected a TypeInstr for 'fnParam'."));
			}

			params->push(ParamInfo(i->type, i->src(), false));
		}

		void RemoveInvalid::fnParamRefTfm(Listing *to, Instr *instr, Nat line) {
			TypeInstr *i = as<TypeInstr>(instr);
			if (!i) {
				throw new (this) InvalidValue(S("Expected a TypeInstr for 'fnParamRef'."));
			}

			params->push(ParamInfo(i->type, i->src(), true));
		}

		void RemoveInvalid::fnCallTfm(Listing *to, Instr *instr, Nat line) {
			TypeInstr *t = as<TypeInstr>(instr);
			if (!t) {
				throw new (this) InvalidValue(S("Using a fnCall that was not created properly."));
			}
			emitFnCall(this, to, t->src(), t->dest(), t->type, false, currentBlock, used->at(line), params);
			params->clear();
		}

		void RemoveInvalid::fnCallRefTfm(Listing *to, Instr *instr, Nat line) {
			TypeInstr *t = as<TypeInstr>(instr);
			if (!t) {
				throw new (this) InvalidValue(S("Using a fnCall that was not created properly."));
			}
			emitFnCall(this, to, t->src(), t->dest(), t->type, true, currentBlock, used->at(line), params);
			params->clear();
		}


		/**
		 * Other instructions.
		 */

		void RemoveInvalid::movTfm(Listing *to, Instr *instr, Nat line) {
			// We need to ensure that at most one operand is a memory operand. Large constants are
			// counted as a memory operand since they need to be loaded separately.

			Operand dest = instr->dest();
			// If dest is a register, we can just load that register.
			if (dest.type() == opRegister) {
				loadRegister(to, dest.reg(), instr->src());
				return;
			}

			// If the source is a register, we can just emit a single store operation regardless.
			Operand src = instr->src();
			if (src.type() == opRegister) {
				*to << instr;
				return;
			}

			// Otherwise, we need to break the instruction into two pieces.
			Reg tmpReg = unusedReg(used->at(line), src.size());
			loadRegister(to, tmpReg, src);
			*to << mov(dest, tmpReg);
		}

		void RemoveInvalid::leaTfm(Listing *to, Instr *instr, Nat line) {
			Operand dest = instr->dest();
			if (dest.type() == opRegister) {
				*to << instr;
				return;
			}

			Reg tmp = unusedReg(used->at(line), dest.size());
			*to << instr->alterDest(tmp);
			*to << mov(dest, tmp);
		}

		void RemoveInvalid::swapTfm(Listing *to, Instr *instr, Nat line) {
			// Note: This is really only needed in the X64 backend, so it is not too important to
			// implement it extremely efficiently here. I think this implementation supports more
			// cases than what is supported on X86 and X64 (there, it is needed to avoid using extra
			// registers, but we don't really have that problem here).

			Operand src = instr->src();
			Operand dest = instr->dest();
			Reg tmp = unusedReg(used->at(line), src.size());
			*to << mov(tmp, src);
			if (src.type() == opRegister || dest.type() == opRegister) {
				*to << mov(src, dest);
			} else {
				used->at(line)->put(tmp);
				Reg tmp2 = unusedReg(used->at(line), src.size());
				*to << mov(tmp2, dest);
				*to << mov(src, tmp2);
			}
			*to << mov(dest, tmp);
		}

		void RemoveInvalid::cmpTfm(Listing *to, Instr *instr, Nat line) {
			// This is like "dataInstr12Tfm" below, but does not attempt to save the result back to
			// memory.

			Operand src = regOrImm(to, instr->src(), line, 12, false);
			Operand dest = instr->dest();
			if (dest.type() != opRegister) {
				// Load the destination.
				Reg t = unusedReg(used->at(line), dest.size());
				loadRegister(to, t, dest);
				*to << instr->alter(t, src);
			} else {
				*to << instr->alterSrc(src);
			}
		}

		void RemoveInvalid::setCondTfm(Listing *to, Instr *instr, Nat line) {
			Operand dest = instr->dest();
			if (dest.type() == opRegister) {
				*to << instr;
			} else {
				Reg tmp = unusedReg(used->at(line), Size::sByte);
				*to << instr->alterDest(tmp);
				*to << mov(dest, tmp);
			}
		}

		void RemoveInvalid::icastTfm(Listing *to, Instr *instr, Nat line) {
			// These work the same.
			ucastTfm(to, instr, line);
		}

		void RemoveInvalid::ucastTfm(Listing *to, Instr *instr, Nat line) {
			// For icast and ucast we allow source operands in memory since there are special
			// instructions that load from memory. Offsets are the same size as for regular loads
			// and stores, so we don't handle them in a special way.

			Operand src = instr->src();
			switch (src.type()) {
			case opRegister:
			case opRelative:
			case opVariable:
				// Supported!
				break;
			default:
				// Add a separate move operation to load the source: we don't support loading
				// arbitrary things. Note: We don't need to update 'used', it is fine if we use the
				// same register for both src and dest!
				Reg r = unusedReg(used->at(line), src.size());
				loadRegister(to, r, src);
				instr = instr->alterSrc(r);
				break;
			}

			Operand dst = instr->dest();
			if (dst.type() != opRegister) {
				Reg t = unusedReg(used->at(line), dst.size());
				*to << instr->alterDest(t);
				*to << mov(dst, t);
			} else {
				*to << instr;
			}
		}

		void RemoveInvalid::fcastTfm(Listing *to, Instr *instr, Nat line) {
			// All operands need to be in fp registers.
			Reg src = loadFpRegister(to, instr->src(), line);

			Operand dest = instr->dest();
			if (dest.type() == opRegister && isVectorReg(dest.reg())) {
				*to << fcast(dest, src);
			} else {
				Reg destReg = unusedVectorReg(used->at(line), dest.size());
				*to << fcast(destReg, src);
				*to << mov(dest, destReg);
			}
		}

		void RemoveInvalid::fcastiTfm(Listing *to, Instr *instr, Nat line) {
			Reg src = loadFpRegister(to, instr->src(), line);

			Operand dest = instr->dest();
			if (dest.type() == opRegister && isIntReg(dest.reg())) {
				*to << instr->alterSrc(src);
			} else {
				Reg destReg = unusedReg(used->at(line), dest.size());
				*to << instr->alter(destReg, src);
				*to << mov(dest, destReg);
			}
		}

		void RemoveInvalid::fcastuTfm(Listing *to, Instr *instr, Nat line) {
			fcastiTfm(to, instr, line);
		}

		void RemoveInvalid::icastfTfm(Listing *to, Instr *instr, Nat line) {
			Operand src = instr->src();
			Reg srcReg = noReg;
			if (src.type() == opRegister && isIntReg(src.reg())) {
				srcReg = src.reg();
			} else {
				srcReg = unusedReg(used->at(line), src.size());
				used->at(line)->put(srcReg);
				loadRegister(to, srcReg, src);
			}

			Operand dest = instr->dest();
			if (dest.type() == opRegister && isVectorReg(dest.reg())) {
				*to << instr->alterSrc(srcReg);
			} else {
				Reg destReg = unusedVectorReg(used->at(line), dest.size());
				*to << instr->alter(destReg, srcReg);
				*to << mov(dest, destReg);
			}
		}

		void RemoveInvalid::ucastfTfm(Listing *to, Instr *instr, Nat line) {
			icastfTfm(to, instr, line);
		}

		void RemoveInvalid::dataInstr12Tfm(Listing *to, Instr *instr, Nat line) {
			// If "dest" is not a register, we need to surround this instruction with a load *and* a
			// store. If "src" is not a register, we need to add a load before. If "src" is too
			// large, we need to make it into a load.
			instr = limitImmSrc(to, instr, line, 12, false);
			removeMemoryRefs(to, instr, line);
		}

		void RemoveInvalid::bitmaskInstrTfm(Listing *to, Instr *instr, Nat line) {
			// If "dest" is not a register, we need to surround this instruction with a load *and* a
			// store. If "src" is not a register, we need to add a load before. If "src" is too
			// large, we need to make it into a load.

			Operand src = instr->src();
			if (src.type() == opConstant) {
				bool large = src.size().size64() > 4;
				Word value = src.constant();

				if (value == 0) {
					// Supported through special case.
				} else if (allOnes(value, large)) {
					// Supported through special case.
				} else if (encodeBitmask(src.constant(), large) == 0) {
					// Not supported, spill to memory.
					instr = instr->alterSrc(largeConstant(src));
				}
			}

			removeMemoryRefs(to, instr, line);
		}

		void RemoveInvalid::shiftInstrTfm(Listing *to, Instr *instr, Nat line) {
			Operand src = instr->src();
			if (src.type() == opConstant) {
				Word value = src.constant();
				if (instr->dest().size().size64() > 4) {
					if (value > 64)
						instr = instr->alterSrc(byteConst(64));
				} else {
					if (value > 32)
						instr = instr->alterSrc(byteConst(32));
				}
			}

			removeMemoryRefs(to, instr, line);
		}

		void RemoveInvalid::dataInstr4RegTfm(Listing *to, Instr *instr, Nat line) {
			Operand src = instr->src();
			if (src.type() != opRegister) {
				Reg t = unusedReg(used->at(line), src.size());
				used->at(line)->put(t);
				loadRegister(to, t, src);
				src = t;
			}

			Operand dest = instr->dest();
			if (dest.type() != opRegister) {
				Reg t = unusedReg(used->at(line), dest.size());
				loadRegister(to, t, dest);
				*to << instr->alter(t, src);
				*to << mov(dest, t);
			} else {
				*to << instr->alterSrc(src);
			}
		}

		Reg RemoveInvalid::loadFpRegister(Listing *to, const Operand &op, Nat line) {
			// Operands need to be in vector registers!
			if (op.type() == opRegister)
				if (isVectorReg(op.reg()))
					return op.reg();

			// Just load it into a free vector register.
			Reg r = unusedVectorReg(used->at(line), op.size());
			used->at(line)->put(r);
			*to << mov(r, op);
			return r;
		}

		void RemoveInvalid::fpInstrTfm(Listing *to, Instr *instr, Nat line) {
			Operand dest = instr->dest();
			DestMode mode = destMode(instr->op());

			Reg srcReg = loadFpRegister(to, instr->src(), line);

			Reg dstReg;
			if (mode & destRead) {
				dstReg = loadFpRegister(to, dest, line);
			} else {
				if (dest.type() == opRegister && isVectorReg(dest.reg())) {
					dstReg = dest.reg();
				} else {
					dstReg = unusedVectorReg(used->at(line), dest.size());
					// We don't need to update the register in the used set either, usage will not overlap.
					// Don't need to load!
				}
			}

			*to << instr->alter(dstReg, srcReg);

			if (destMode(instr->op()) & destWrite) {
				if (dest.type() != opRegister || dest.reg() != dstReg)
					*to << mov(dest, dstReg);
			}
		}

		void RemoveInvalid::idivTfm(Listing *to, Instr *instr, Nat line) {
			Operand src = instr->src();
			if (src.type() != opRegister) {
				Reg t = unusedReg(used->at(line), src.size());
				used->at(line)->put(t);
				loadRegister(to, t, src);
				src = t;
			}

			*to << cmp(src, xConst(src.size(), 0));
			*to << jmp(divZeroLabel(to), ifEqual); // TODO: Could be marked as "robust" to aid branch predictor

			Operand dest = instr->dest();
			if (dest.type() != opRegister) {
				Reg t = unusedReg(used->at(line), dest.size());
				loadRegister(to, t, dest);
				*to << instr->alter(t, src);
				*to << mov(dest, t);
			} else {
				*to << instr->alterSrc(src);
			}
		}

		void RemoveInvalid::udivTfm(Listing *to, Instr *instr, Nat line) {
			idivTfm(to, instr, line);
		}

		void RemoveInvalid::imodTfm(Listing *to, Instr *instr, Nat line) {
			umodTfm(to, instr, line);
		}

		void RemoveInvalid::umodTfm(Listing *to, Instr *instr, Nat line) {
			// GCC generates the following code for mod:
			// div <t>, <a>, <b>
			// mul <t>, <t>, <b>
			// sub <out>, <a>, <t>

			// Load src if needed.
			Operand src = instr->src();
			if (src.type() != opRegister) {
				Reg t = unusedReg(used->at(line), src.size());
				used->at(line)->put(t);
				loadRegister(to, t, src);
				src = t;
			}

			// Load dest if needed.
			Operand dest = instr->dest();
			if (dest.type() != opRegister) {
				Reg t = unusedReg(used->at(line), dest.size());
				used->at(line)->put(t);
				loadRegister(to, t, dest);
				dest = t;
			}

			*to << cmp(src, xConst(src.size(), 0));
			*to << jmp(divZeroLabel(to), ifEqual); // TODO: Could be marked as "robust" to aid branch predictor

			// The modulo code. Since we can't use 3-op codes here, we generate:
			// mov <t>, <a>
			// div <t>, <b>
			// mul <t>, <b>
			// sub <a>, <t>

			Reg t = unusedReg(used->at(line), dest.size());
			*to << mov(t, dest);
			if (instr->op() == op::imod)
				*to << idiv(t, src);
			else
				*to << udiv(t, src);
			*to << mul(t, src);
			*to << sub(dest, t);

			// Store dest if needed.
			if (dest != instr->dest())
				*to << mov(instr->dest(), dest);

		}

	}
}
