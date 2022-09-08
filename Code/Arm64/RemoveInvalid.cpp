#include "stdafx.h"
#include "RemoveInvalid.h"
#include "../Listing.h"
#include "../UsedRegs.h"
#include "../Exception.h"

namespace code {
	namespace arm64 {

#define TRANSFORM(x) { op::x, &RemoveInvalid::x ## Tfm }

		const OpEntry<RemoveInvalid::TransformFn> RemoveInvalid::transformMap[] = {
			TRANSFORM(fnCall),
			TRANSFORM(fnCallRef),
			TRANSFORM(fnParam),
			TRANSFORM(fnParamRef),
			TRANSFORM(mov),
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


		void RemoveInvalid::prologTfm(Listing *dest, Instr *instr, Nat line) {
			currentBlock = dest->root();
			*dest << instr;
		}

		void RemoveInvalid::beginBlockTfm(Listing *dest, Instr *instr, Nat line) {
			currentBlock = instr->src().block();
			*dest << instr;
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

		void RemoveInvalid::movTfm(Listing *dest, Instr *instr, Nat line) {
			if (instr->src().type() == opConstant) {
				if (instr->src().constant() > 0xFFFF) {
					large->push(instr->src());
					// TODO: Make sure that the destination is a register.
					*dest << instr->alterSrc(xRel(large->last().size(), lblLarge, Offset::sWord*(large->count()-1)));
					return;
				}
			}

			*dest << instr;
		}

	}
}
