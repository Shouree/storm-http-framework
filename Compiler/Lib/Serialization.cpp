#include "stdafx.h"
#include "Serialization.h"
#include "Compiler/Exception.h"
#include "Compiler/Type.h"
#include "Compiler/Engine.h"
#include "Core/Handle.h"
#include "Core/Io/Serialization.h"
#include "Code/Instr.h"
#include "Code/Listing.h"

namespace storm {

	SerializeInfo::SerializeInfo(Function *read, Function *write)
		: read(read), write(write) {}

	MAYBE(SerializeInfo *) serializeInfo(Type *type) {
		Engine &e = type->engine;
		Value me = thisPtr(type);
		Value oStream(StormInfo<ObjOStream>::type(e));
		Value iStream(StormInfo<ObjIStream>::type(e));
		Scope scope = e.scope();

		Function *read = null;
		Function *write = null;

		{
			SimplePart *name = new (e) SimplePart(S("read"));
			name->params->push(iStream);
			if (Function *found = as<Function>(type->findHere(name, scope))) {
				if (Value(type).mayReferTo(found->result))
					read = found;
			}
		}

		{
			SimplePart *name = new (e) SimplePart(S("write"));
			name->params->push(me);
			name->params->push(oStream);
			// We ignore the return type here. It is not important!
			write = as<Function>(type->findHere(name, scope));
		}

		if (read && write)
			return new (type) SerializeInfo(read, write);
		else
			return null;
	}

	Bool serializable(Type *type) {
		return serializeInfo(type) != null;
	}

	Function *serializedTypeFn(SerializedType *type) {
		Engine &e = type->engine();
		Value result(StormInfo<SerializedType>::type(e));

		using namespace code;
		Listing *l = new (e) Listing(false, result.desc(e));
		*l << prolog();
		*l << fnRet(objPtr(type));

		Array<Value> *params = new (e) Array<Value>();
		Function *fn = new (e) Function(result, new (e) Str(S("serializedType")), params);
		fn->make(fnStatic);
		fn->setCode(new (e) DynamicCode(l));
		return fn;
	}

	Function *serializedReadFn(Type *type) {
		using namespace code;
		Engine &e = type->engine;

		Value result(type);
		Value streamType(StormInfo<ObjIStream>::type(e));
		Value typeType(StormInfo<Type>::type(e));

		Listing *l = new (e) Listing(false, result.desc(e));
		code::Var stream = l->createParam(streamType.desc(e));

		*l << prolog();
		if (result.isObject()) {
			Function *fn = findStormMemberFn(streamType, S("readClass"), typeType);

			*l << fnParam(streamType.desc(e), stream);
			*l << fnParam(typeType.desc(e), type->typeRef());
			*l << fnCall(fn->ref(), true, result.desc(e), ptrA);
			*l << fnRet(ptrA);
		} else {
			code::Var tmp = l->createVar(l->root(), result.size());
			Type *unkType = as<Type>(e.scope().find(parseSimpleName(e, S("core.lang.unknown.PTR_GC"))));
			if (!unkType)
				throw new (type) InternalError(S("The type core.lang.unknown.PTR_GC does not exist!"));
			Function *fn = findStormMemberFn(streamType, S("readValue"), typeType, Value(unkType));

			*l << lea(ptrA, tmp);
			*l << fnParam(streamType.desc(e), stream);
			*l << fnParam(typeType.desc(e), type->typeRef());
			*l << fnParam(e.ptrDesc(), ptrA);
			*l << fnCall(fn->ref(), true);
			*l << fnRet(tmp);
		}

		Array<Value> *params = new (e) Array<Value>(1, streamType);
		Function *fn = new (e) Function(result, new (e) Str(S("read")), params);
		fn->make(fnStatic);
		fn->setCode(new (e) DynamicCode(l));
		return fn;
	}

}
