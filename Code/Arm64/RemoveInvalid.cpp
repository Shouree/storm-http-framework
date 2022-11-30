#include "stdafx.h"
#include "RemoveInvalid.h"
#include "Asm.h"
#include "../Listing.h"
#include "../UsedRegs.h"
#include "../Exception.h"

namespace code {
	namespace arm64 {

#define TRANSFORM(x) { op::x, &RemoveInvalid::x ## Tfm }
#define DATA12(x) { op::x, &RemoveInvalid::dataInstr12Tfm }
#define BITMASK(x) { op::x, &RemoveInvalid::bitmaskInstrTfm }
#define DATA4REG(x) { op::x, &RemoveInvalid::dataInstr4RegTfm }

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
			TRANSFORM(swap),
			TRANSFORM(cmp),
			TRANSFORM(setCond),
			TRANSFORM(icast),
			TRANSFORM(ucast),

			DATA12(add),
			DATA12(sub),
			DATA4REG(mul),

			BITMASK(band),
			BITMASK(bor),
			BITMASK(bxor),
			// not?
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

			Instr *i = src->at(line);

			// TODO: Check if we need to alter the insn further.

			TransformFn f = t[i->op()];
			if (f) {
				(this->*f)(dest, i, line);
			} else {
				*dest << i;
			}
		}

		void RemoveInvalid::after(Listing *dest, Listing *src) {
			for (Nat i = 0; i < large->count(); i++) {
				*dest << alignAs(Size::sPtr);
				if (i == 0)
					*dest << lblLarge;
				*dest << dat(large->at(i));
			}
		}

		Operand RemoveInvalid::largeConstant(const Operand &data) {
			Nat offset = large->count();
			large->push(data);
			return xRel(data.size(), lblLarge, Offset::sWord * offset);
		}


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
			emitFnCall(to, t->src(), t->dest(), t->type, false, currentBlock, used->at(line), params);
			params->clear();
		}

		void RemoveInvalid::fnCallRefTfm(Listing *to, Instr *instr, Nat line) {
			TypeInstr *t = as<TypeInstr>(instr);
			if (!t) {
				throw new (this) InvalidValue(S("Using a fnCall that was not created properly."));
			}

			emitFnCall(to, t->src(), t->dest(), t->type, true, currentBlock, used->at(line), params);
			params->clear();
		}

		void RemoveInvalid::movTfm(Listing *to, Instr *instr, Nat line) {
			// We need to ensure that at most one operand is a memory operand. Large constants are
			// count as a memory operand since they need to be loaded separately.

			Operand src = instr->src();
			switch (src.type()) {
			case opRegister:
				// No problem, we will be able to handle this instruction without issues.
				*to << instr;
				return;
			case opConstant:
				// If the constant is too large, we need to use a load instruction.
				if (src.constant() > 0xFFFF) {
					src = largeConstant(src);
					instr = instr->alterSrc(src);
					// Let execution continue in order to handle the case where destination is not a register.
				}
				break;
			}

			// If we get here, we need to try to make "dest" into a register if it is not already.
			if (instr->dest().type() == opRegister) {
				*to << instr;
			} else {
				Reg tmpReg = unusedReg(used->at(line), src.size());
				*to << instr->alterDest(tmpReg);
				*to << instr->alterSrc(tmpReg);
			}
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

			// If "dest" is not a register, we need to surround this instruction with a load *and* a
			// store. If "src" is not a register, we need to add a load before. If "src" is too
			// large, we need to make it into a load.
			Operand src = instr->src();
			if (src.type() == opConstant) {
				if (src.constant() > 0xFFF) {
					src = largeConstant(src);
				}
			}

			switch (src.type()) {
			case opRegister:
			case opConstant:
				// TODO: More things to ignore here!
				break;
			default:
				// Emit a load first.
				Reg t = unusedReg(used->at(line), src.size());
				used->at(line)->put(t);
				*to << mov(t, src);
				src = t;
				break;
			}

			Operand dest = instr->dest();
			if (dest.type() != opRegister) {
				// Load and store the destination.
				Reg t = unusedReg(used->at(line), dest.size());
				*to << mov(t, dest);
				*to << instr->alter(t, src);
			} else {
				*to << instr->alterSrc(src);
			}
		}

		void RemoveInvalid::setCondTfm(Listing *to, Instr *instr, Nat line) {
			Operand dest = instr->dest();
			if (dest.type() != opRegister) {
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
				*to << mov(r, src);
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

		void RemoveInvalid::dataInstr12Tfm(Listing *to, Instr *instr, Nat line) {
			// If "dest" is not a register, we need to surround this instruction with a load *and* a
			// store. If "src" is not a register, we need to add a load before. If "src" is too
			// large, we need to make it into a load.
			Operand src = instr->src();
			if (src.type() == opConstant) {
				if (src.constant() > 0xFFF) {
					src = largeConstant(src);
				}
			}

			switch (src.type()) {
			case opRegister:
			case opConstant:
				// TODO: More things to ignore here!
				break;
			default:
				// Emit a load first.
				Reg t = unusedReg(used->at(line), src.size());
				used->at(line)->put(t);
				*to << mov(t, src);
				src = t;
				break;
			}

			Operand dest = instr->dest();
			if (dest.type() != opRegister) {
				// Load and store the destination.
				Reg t = unusedReg(used->at(line), dest.size());
				*to << mov(t, dest);
				*to << instr->alter(t, src);
				*to << mov(dest, t);
			} else {
				*to << instr->alterSrc(src);
			}
		}

		void RemoveInvalid::bitmaskInstrTfm(Listing *to, Instr *instr, Nat line) {
			// If "dest" is not a register, we need to surround this instruction with a load *and* a
			// store. If "src" is not a register, we need to add a load before. If "src" is too
			// large, we need to make it into a load.

			Operand src = instr->src();
			if (src.type() == opConstant) {
				bool large = src.size().size64() > 4;
				if (encodeBitmask(src.constant(), large) == 0) {
					src = largeConstant(src);
				}
			}

			switch (src.type()) {
			case opRegister:
			case opConstant:
				// TODO: More things to ignore here!
				break;
			default:
				// Emit a load first.
				Reg t = unusedReg(used->at(line), src.size());
				used->at(line)->put(t);
				*to << mov(t, src);
				src = t;
				break;
			}

			Operand dest = instr->dest();
			if (dest.type() != opRegister) {
				// Load and store the destination.
				Reg t = unusedReg(used->at(line), dest.size());
				*to << mov(t, dest);
				*to << instr->alter(t, src);
				*to << mov(dest, t);
			} else {
				*to << instr->alterSrc(src);
			}
		}

		void RemoveInvalid::dataInstr4RegTfm(Listing *to, Instr *instr, Nat line) {
			Operand src = instr->src();
			if (src.type() != opRegister) {
				Reg t = unusedReg(used->at(line), src.size());
				used->at(line)->put(t);
				*to << mov(t, src);
				src = t;
			}

			Operand dest = instr->dest();
			if (dest.type() != opRegister) {
				Reg t = unusedReg(used->at(line), dest.size());
				*to << mov(t, dest);
				*to << instr->alter(t, src);
				*to << mov(dest, t);
			} else {
				*to << instr->alterSrc(src);
			}
		}

	}
}
