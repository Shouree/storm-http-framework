#include "stdafx.h"
#include "ToS.h"
#include "Package.h"
#include "Scope.h"
#include "Type.h"
#include "Exception.h"
#include "Engine.h"

namespace storm {

	ToSTemplate::ToSTemplate() : Template(new (engine()) Str(S("toS"))) {}

	MAYBE(Named *) ToSTemplate::generate(SimplePart *part) {
		Array<Value> *params = part->params;
		if (params->count() != 1)
			return null;

		Value type = params->at(0).asRef(false);

		// Only ever needed for value types or built-in types.
		if (!type.isValue())
			return null;

		if (type.type == null)
			return null;

		// Find the relevant output operator.
		Scope root = engine().scope();
		Function *found = null;
		SimplePart *output = new (this) SimplePart(new (this) Str(L"<<"));
		output->params->push(thisPtr(StrBuf::stormType(engine())));
		output->params->push(type);

		// Look in the parent package.
		if (Package *pkg = ScopeLookup::firstPkg(type.type)) {
			found = as<Function>(pkg->find(output, root));
		}

		// If not found there, look directly inside StrBuf.
		if (!found) {
			found = as<Function>(StrBuf::stormType(engine())->find(output, root));
		}

		// For enums, it might also be located in 'core'.
		if (!found) {
			found = as<Function>(engine().package(S("core"))->find(output, root));
		}

		// If not found anywhere, do not create a toS!
		if (!found)
			return null;

		return new (this) ToSFunction(found);
	}


	ToSFunction::ToSFunction(Function *use)
		: Function(Value(StormInfo<Str>::type(use->engine())),
				new (use) Str(S("toS")),
				new (use) Array<Value>(1, use->params->at(1).asRef(false))),
		  use (use) {

		setCode(new (engine()) LazyCode(fnPtr(engine(), &ToSFunction::create, this)));
	}

	CodeGen *ToSFunction::create() {
		using namespace code;
		Value type = params->at(0);

		CodeGen *to = new (this) CodeGen(runOn(), isMember(), Value(Str::stormType(engine())));
		Var param = to->createParam(type);

		*to->l << prolog();

		Type *strBufT = StrBuf::stormType(engine());
		Var strBuf = allocObject(to, strBufT);

		// Call the output operator.
		Array<Operand> *p = new (this) Array<Operand>();
		p->push(strBuf);
		if (use->params->at(1).ref) {
			*to->l << lea(ptrA, param);
			p->push(ptrA);
		} else {
			p->push(param);
		}
		use->localCall(to, p, new (this) CodeResult(), false);

		// Call 'toS' on the StrBuf to get the string.
		Function *toS = as<Function>(strBufT->find(S("toS"), thisPtr(strBufT), engine().scope()));
		if (!toS)
			throw new (this) InternalError(S("Can not find 'toS' for the StrBuf type!"));

		CodeResult *str = new (this) CodeResult(result, to->l->root());
		p = new (this) Array<Operand>();
		p->push(strBuf);
		toS->localCall(to, p, str, false);

		to->returnValue(str->location(to));
		return to;
	}

}
