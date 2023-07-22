#include "stdafx.h"
#include "Adapter.h"
#include "CodeGen.h"
#include "Engine.h"

namespace storm {
	using namespace code;

	const void *makeRefParams(Function *wrap) {
		assert(wrap->result.returnInReg(), L"Currently, returning values is not supported.");

		if (InlineCode *code = as<InlineCode>(wrap->getCode())) {
			TODO(L"Inline the code!");
		}

		CodeGen *s = new (wrap) CodeGen(wrap->runOn(), wrap->isMember(), wrap->result);
		Engine &e = wrap->engine();

		Array<Var> *params = new (wrap) Array<Var>();
		for (Nat i = 0; i < wrap->params->count(); i++) {
			params->push(s->createParam(wrap->params->at(i).asRef(true)));
		}

		Var result = s->createVar(wrap->result).v;

		*s->l << prolog();

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
