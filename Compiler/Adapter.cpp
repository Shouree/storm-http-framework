#include "stdafx.h"
#include "Adapter.h"
#include "CodeGen.h"
#include "Engine.h"

namespace storm {
	using namespace code;

	code::Binary *makeRefParams(Function *wrap) {
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

		return new (wrap) Binary(wrap->engine().arena(), s->l);
	}

	Bool allRefParams(Function *fn) {
		for (Nat i = 0; i < fn->params->count(); i++)
			if (!fn->params->at(i).ref)
				return false;

		return true;
	}

	code::Binary *makeToSThunk(Function *fn) {
		assert(fn->params->count() == 1);

		Type *strBufT = StrBuf::stormType(fn->engine());
		SimplePart *part = new (fn) SimplePart(S("<<"));
		part->params->push(thisPtr(strBufT));
		part->params->push(Str::stormType(fn->engine()));
		Function *strBufAppend = as<Function>(strBufT->find(part, Scope()));
		if (!strBufAppend)
			throw new (fn) InternalError(S("Failed to find <<(StrBuf, Str)!"));

		Engine &e = fn->engine();
		TypeDesc *ptr = e.ptrDesc();
		Listing *l = new (fn) Listing(false, e.voidDesc());

		Var valRef = l->createParam(ptr);
		Var strBuf = l->createParam(ptr);

		*l << prolog();

		// Call 'fn' (i.e. toS()):
		*l << fnParam(ptr, valRef);
		*l << fnCall(fn->ref(), true, ptr, ptrA);

		// Call '<<' to add it to the string buffer:
		*l << fnParam(ptr, strBuf);
		*l << fnParam(ptr, ptrA);
		*l << fnCall(strBufAppend->ref(), true);

		*l << fnRet();

		return new (fn) code::Binary(fn->engine().arena(), l);
	}

}
