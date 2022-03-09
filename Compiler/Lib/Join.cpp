#include "stdafx.h"
#include "Join.h"
#include "Array.h"
#include "Fn.h"
#include "Compiler/Function.h"
#include "Compiler/Exception.h"
#include "Compiler/Engine.h"
#include "Code/Listing.h"

namespace storm {

	Join::Join(ArrayBase *array, Str *separator)
		: array(array), separator(separator), lambda(null), thunk(null) {}

	Join::Join(ArrayBase *array, Str *separator, FnBase *lambda, void *lambdaCall, os::CallThunk thunk)
		: array(array), separator(separator), lambda(lambda), lambdaCall(lambdaCall), thunk(thunk) {}

	void Join::toS(StrBuf *to) const {
		if (lambda && thunk) {
			for (Nat i = 0; i < array->count(); i++) {
				if (i > 0 && separator)
					*to << separator;

				void *params[2] = { (void *)&lambda, array->getRaw(i) };
				Str *result = null;
				(*thunk)(lambdaCall, true, params, null, &result);
				*to << result;
			}
		} else {
			const Handle &handle = array->handle;

			for (Nat i = 0; i < array->count(); i++) {
				if (i > 0 && separator)
					*to << separator;
				(*handle.toSFn)(array->getRaw(i), to);
			}
		}
	}

	Join *CODECALL joinBase(ArrayBase *array) {
		return new (array) Join(array, null);
	}

	Join *CODECALL joinSeparator(ArrayBase *array, Str *separator) {
		return new (array) Join(array, separator);
	}

	Join *CODECALL createJoin(ArrayBase *array, Str *separator, FnBase *lambda, void *lambdaCall, os::CallThunk thunk) {
		return new (array) Join(array, separator, lambda, lambdaCall, thunk);
	}


	JoinTemplate::JoinTemplate() : Template(new (engine()) Str(S("join"))) {}

	MAYBE(Named *) JoinTemplate::generate(SimplePart *part) {
		Array<Value> *params = part->params;
		if (params->count() < 1)
			return null;
		if (params->count() > 3)
			return null;

		Value array = params->at(0).asRef(false);
		Value element = unwrapArray(array).asRef(false);
		// It wasn't an array - type did not change.
		if (array.type == element.type)
			return null;

		Engine &e = engine();
		Type *strType = Str::stormType(e);
		Type *joinType = Join::stormType(e);

		if (params->count() == 1) {
			// join(<array>)
			Array<Value> *params = new (this) Array<Value>(1, Value());
			params->at(0) = array;
			return nativeFunction(e, Value(joinType), S("join"), params, address(&joinBase));
		} else if (params->count() == 2 && params->at(1).type == strType) {
			// join(<array>, <string>)
			Array<Value> *params = new (this) Array<Value>(2, Value());
			params->at(0) = array;
			params->at(1) = Value(strType);
			return nativeFunction(e, Value(joinType), S("join"), params, address(&joinSeparator));
		} else {
			// either: join(<array>, <lambda>) or join(<array>, <string>, <lambda>)
			if (params->count() == 3 && params->at(1).type != strType)
				return null;

			// TODO: We should verify the last parameter. Otherwise, multiple join calls with lambdas will fail.

			// Just generate what we need, otherwise auto-inference does not work.
			Array<Value> *fnParams = new (this) Array<Value>(2, Value());
			fnParams->at(0) = Value(strType);
			fnParams->at(1) = element.asRef(false);
			Type *f = fnType(fnParams);

			Array<Value> *par = new (this) Array<Value>(*params);
			par->at(0) = array;
			par->last() = Value(f);

			Function *lambdaCall = as<Function>(f->find(S("call"), valList(e, 2, Value(f), element), Scope()));
			if (!lambdaCall)
				throw new (this) InternalError(S("Unable to find the 'call' member of a function object."));

			code::Listing *l = new (this) code::Listing(false, e.ptrDesc());
			{
				using namespace code;

				TypeDesc *ptrDesc = e.ptrDesc();

				Var array = l->createParam(ptrDesc);
				Operand sep = ptrConst(0);
				if (params->count() == 3)
					sep = l->createParam(ptrDesc);
				Var lambda = l->createParam(ptrDesc);

				*l << prolog();
				*l << fnParam(ptrDesc, array);
				*l << fnParam(ptrDesc, sep);
				*l << fnParam(ptrDesc, lambda);
				*l << fnParam(ptrDesc, lambdaCall->ref());
				*l << fnParam(ptrDesc, lambdaCall->thunkRef());
				*l << fnCall(e.ref(builtin::createJoin), false, ptrDesc, ptrA);
				*l << fnRet(ptrA);
			}

			return dynamicFunction(e, Value(joinType), S("join"), par, l);
		}

		// Should not happen:
		return null;
	}

}
