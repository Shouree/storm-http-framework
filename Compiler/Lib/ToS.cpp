#include "stdafx.h"
#include "ToS.h"
#include "Package.h"
#include "Scope.h"
#include "Type.h"
#include "Exception.h"
#include "Engine.h"

namespace storm {

	/**
	 * Generation of toS() from toS(StrBuf):
	 */


	ToSTemplate::ToSTemplate() : Template(new (engine()) Str(S("toS"))) {}

	MAYBE(Named *) ToSTemplate::generate(SimplePart *part) {
		Array<Value> *params = part->params;
		if (params->count() != 1)
			return null;

		Value type = params->at(0);

		// Only ever needed for value types or built-in types.
		if (!type.isValue())
			return null;

		if (type.type == null)
			return null;

		// Find a toS(StrBuf) function for the type:
		SimplePart *toFind = new (this) SimplePart(S("toS"));
		toFind->params->push(thisPtr(type.type));
		toFind->params->push(StrBuf::stormType(engine()));

		// We only examine member functions of the type:
		if (Function *found = as<Function>(type.type->find(toFind, engine().scope())))
			return new (this) ToSFunction(found);

		// Otherwise, nothing to generate.
		return null;
	}


	ToSFunction::ToSFunction(Function *use)
		: Function(Value(StormInfo<Str>::type(use->engine())),
				new (use) Str(S("toS")),
				new (use) Array<Value>(1, thisPtr(use->params->at(0).type))),
		  use (use) {

		setCode(new (engine()) LazyCode(fnPtr(engine(), &ToSFunction::create, this)));

		// Add a listener to the target type and remove ourselves whenever that function gains a toS
		// function.
		use->params->at(0).type->watchAdd(this);
	}

	void ToSFunction::notifyAdded(NameSet *to, Named *added) {
		if (*added->name == S("toS") && added->params->count() == 1 && added->params->at(0).type == to) {
			// Found a toS function that was added. Remove ourselves from our parent:
			if (NameSet *inside = as<NameSet>(parentLookup)) {
				inside->remove(this);
				to->watchRemove(this);
			}
		}
	}

	CodeGen *ToSFunction::create() {
		using namespace code;
		Value type = params->at(0);

		CodeGen *to = new (this) CodeGen(use->runOn(), isMember(), Value(Str::stormType(engine())));
		Var param = to->createParam(type);

		*to->l << prolog();

		Type *strBufT = StrBuf::stormType(engine());
		Var strBuf = allocObject(to, strBufT);

		// Call the toS function:
		Array<Operand> *p = new (this) Array<Operand>();
		p->push(param);
		p->push(strBuf);
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



	/**
	 * Generation of '<<' from 'toS()' or 'toS(StrBuf)':
	 */

	OutputOperatorTemplate::OutputOperatorTemplate() : Template(new (engine()) Str(S("<<"))) {}

	MAYBE(Named *) OutputOperatorTemplate::generate(SimplePart *part) {
		Array<Value> *params = part->params;
		if (params->count() != 2)
			return null;

		Type *strBufType = StrBuf::stormType(engine());
		// Note: We accept subtypes to StrBuf if anyone subclasses it.
		if (!Value(strBufType).mayStore(params->at(0).asRef(false)))
			return null;

		if (params->at(0) != strBufType)
			return null;

		// Only needed for value types:
		Value type = params->at(1);
		if (!type.type || !type.isValue())
			return null;

		// Find a toS(StrBuf), this is prioritized if it exists:
		SimplePart *toFind = new (this) SimplePart(S("toS"));
		toFind->params->push(thisPtr(type.type));
		toFind->params->push(Value(strBufType));

		// We only need to look inside the type:
		if (Function *found = as<Function>(type.type->find(toFind, engine().scope())))
			return new (this) OutputOperator(found);

		// If we did not find it, try without the StrBuf parameter:
		toFind->params->pop();
		if (Function *found = as<Function>(type.type->find(toFind, engine().scope())))
			return new (this) OutputOperator(found);

		// Otherwise, give up.
		return null;
	}


	OutputOperator::OutputOperator(Function *use)
		: Function(Value(StrBuf::stormType(use->engine())),
				new (use) Str(S("<<")),
				valList(use->engine(), 2, Value(StrBuf::stormType(use->engine())), use->params->at(0))),
		  use(use) {

		setCode(new (engine()) LazyCode(fnPtr(engine(), &OutputOperator::create, this)));
	}

	CodeGen *OutputOperator::create() {
		using namespace code;

		Value strBuf = params->at(0);
		Value srcType = params->at(1);

		CodeGen *to = new (this) CodeGen(runOn(), isMember(), strBuf);
		Var strBufParam = to->createParam(strBuf);
		Var srcParam = to->createParam(srcType);

		// Two options, either 'toS()' or 'toS(StrBuf)':
		if (use->params->count() == 2) {
			// toS(StrBuf):
			Array<Operand> *ops = new (this) Array<Operand>();
			ops->push(srcParam);
			ops->push(strBufParam);
			use->autoCall(to, ops, new (this) CodeResult());
		} else {
			// toS():
			Value strType(Str::stormType(engine()));
			CodeResult *strResult = new (this) CodeResult(strType, to->l->root());

			Array<Operand> *ops = new (this) Array<Operand>();
			ops->push(srcParam);
			use->autoCall(to, ops, strResult);

			// Find the regular <<(Str) operator:
			Array<Value> *params = new (this) Array<Value>(2, thisPtr(strBuf.type));
			params->at(1) = strType;
			Function *outputStr = as<Function>(strBuf.type->find(S("<<"), params, engine().scope()));
			if (!outputStr)
				throw new (this) InternalError(S("Can not find '<<(StrBuf, Str)' in StrBuf!"));

			ops = new (this) Array<Operand>();
			ops->push(strBufParam);
			ops->push(strResult->location(to));
			outputStr->autoCall(to, ops, new (this) CodeResult());
		}

		to->returnValue(strBufParam);
		return to;
	}
}
