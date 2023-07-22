#include "stdafx.h"
#include "Adapter.h"
#include "CodeGen.h"
#include "Engine.h"

namespace storm {
	using namespace code;

	const void *makeRefParams(Function *wrap) {
		assert(wrap->result.returnInReg(), L"Currently, returning values is not supported.");

		CodeGen *s = new (wrap) CodeGen(wrap->runOn(), wrap->isMember(), wrap->result);
		Engine &e = wrap->engine();

		Array<Var> *params = new (wrap) Array<Var>();
		for (Nat i = 0; i < wrap->params->count(); i++) {
			params->push(s->createParam(wrap->params->at(i).asRef(true)));
		}

		Var result = s->createVar(wrap->result).v;

		*s->l << prolog();

		// If possible to inline, do that!
		if (InlineCode *code = as<InlineCode>(wrap->getCode())) {
			Array<code::Operand> *actuals = new (wrap) Array<code::Operand>();
			actuals->reserve(wrap->params->count());

			for (Nat i = 0; i < wrap->params->count(); i++) {
				Value type = wrap->params->at(i);
				if (type.ref) {
					// Nothing special needs to be done...
					actuals->push(params->at(i));
				} else {
					// Create a copy. TODO: We could probably make an Operand that refers to the
					// value with some care.
					VarInfo t = s->createVar(type);

					if (type.isAsmType()) {
						*s->l << mov(ptrA, params->at(i));
						*s->l << mov(t.v, xRel(t.v.size(), ptrA, Offset()));
					} else {
						*s->l << lea(ptrA, t.v);
						*s->l << fnParam(e.ptrDesc(), ptrA);
						*s->l << fnParam(e.ptrDesc(), params->at(i));
						*s->l << fnCall(type.copyCtor(), true);
					}

					t.created(s);
					actuals->push(t.v);
				}
			}

			code->code(s, actuals, new (wrap) CodeResult(wrap->result, result));

		} else {

			for (Nat i = 0; i < wrap->params->count(); i++) {
				Value type = wrap->params->at(i);
				if (type.ref) {
					// Nothing special needs to be done...
					*s->l << fnParam(type.desc(e), params->at(i));
				} else {
					// Copy the reference into the 'real' value.
					*s->l << fnParamRef(type.desc(e), params->at(i));
				}
			}

			*s->l << fnCall(wrap->ref(), wrap->isMember(), wrap->result.desc(e), result);

		}

		*s->l << fnRet(result);

		Binary *b = new (wrap) Binary(wrap->engine().arena(), s->l);
		return b->address();
	}

	Bool allRefParams(Function *fn) {
		for (Nat i = 0; i < fn->params->count(); i++)
			if (!fn->params->at(i).ref)
				return false;

		return true;
	}

}
