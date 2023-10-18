#include "stdafx.h"
#include "Set.h"
#include "Maybe.h"
#include "Core/Str.h"
#include "Core/Set.h"
#include "Compiler/Type.h"
#include "Compiler/Engine.h"
#include "Serialization.h"
#include "Fn.h"

namespace storm {

	Type *createSet(Str *name, ValueArray *params) {
		if (params->count() != 1)
			return null;

		Value k = params->at(0);
		if (k.ref)
			return null;

		return new (params) SetType(name, k.type, false);
	}

	Type *createRefSet(Str *name, ValueArray *params) {
		if (params->count() != 1)
			return null;

		Value k = params->at(0);
		if (k.ref)
			return null;
		if (k.isValue())
			return null;

		return new (params) SetType(name, k.type, true);
	}

	SetType::SetType(Str *name, Type *k, Bool ref) : Type(name, typeClass), k(k), ref(ref) {
		params = new (engine) Array<Value>(Value(k));

		setSuper(SetBase::stormType(engine));
	}

	void CODECALL SetType::createClass(void *mem) {
		SetType *t = (SetType *)runtime::typeOf((RootObject *)mem);
		SetBase *o = new (Place(mem)) SetBase(t->k->handle());
		runtime::setVTable(o);
	}

	void CODECALL SetType::createRefClass(void *mem) {
		SetType *t = (SetType *)runtime::typeOf((RootObject *)mem);
		SetBase *o = new (Place(mem)) SetBase(t->engine.refObjHandle());
		runtime::setVTable(o);
	}

	void CODECALL SetType::copyClass(void *mem, SetBase *src) {
		SetBase *o = new (Place(mem)) SetBase(*src);
		runtime::setVTable(o);
	}

	Bool SetType::loadAll() {
		Engine &e = engine;

		Value t = thisPtr(this);
		Value key = Value(k);
		Value keyRef = key.asRef();
		Value iter = Value(new (this) SetIterType(k));
		Value boolT = Value(StormInfo<Bool>::type(e));

		Array<Value> *thisKey = valList(e, 2, t, keyRef);

		add(iter.type);
		if (ref) {
			add(nativeFunction(e, Value(), Type::CTOR, valList(e, 1, t), address(&SetType::createRefClass)));
		} else {
			add(nativeFunction(e, Value(), Type::CTOR, valList(e, 1, t), address(&SetType::createClass)));
		}
		add(nativeFunction(e, Value(), Type::CTOR, valList(e, 2, t, t), address(&SetType::copyClass)));
		add(nativeFunction(e, boolT, S("put"), thisKey, address(&SetBase::putRaw)));
		add(nativeFunction(e, Value(), S("put"), valList(e, 2, t, t), address(&SetBase::putSetRaw)));
		add(nativeFunction(e, boolT, S("has"), thisKey, address(&SetBase::hasRaw)));
		add(nativeFunction(e, keyRef, S("get"), thisKey, address(&SetBase::getRaw)));
		add(nativeFunction(e, keyRef, S("[]"), thisKey, address(&SetBase::atRaw)));
		add(nativeFunction(e, boolT, S("remove"), thisKey, address(&SetBase::removeRaw)));
		add(nativeFunction(e, iter, S("begin"), valList(e, 1, t), address(&SetBase::beginRaw)));
		add(nativeFunction(e, iter, S("end"), valList(e, 1, t), address(&SetBase::endRaw)));
		addMaybeAccess();

		if (!k->isA(StormInfo<TObject>::type(e))) {
			if (SerializeInfo *info = serializeInfo(k)) {
				addSerialization(info);
			} else {
				watchFor |= watchSerialization;
			}
		}

		if (watchFor)
			k->watchAdd(this);

		return Type::loadAll();
	}

	void SetType::addMaybeAccess() {
		Value maybe = wrapMaybe(Value(k));

		using namespace code;
		Listing *l = new (this) Listing(true, maybe.desc(engine));

		TypeDesc *ptr = engine.ptrDesc();
		Var me = l->createParam(ptr);
		Var key = l->createParam(ptr);

		Var result = l->createVar(l->root(), maybe.desc(engine), freeOnBoth | freeInactive);

		*l << prolog();

		// Call Map::getUnsafeRaw:
		*l << fnParam(ptr, me);
		*l << fnParam(ptr, key);
		*l << fnCall(engine.ref(builtin::setGetUnsafe), true, ptr, ptrA);

		// Check the return value:
		Label hasValue = l->label();

		*l << cmp(ptrA, ptrConst(0));
		*l << jmp(hasValue, ifNotEqual);

		// It was null, we can just return it, since it was initialized to null:
		*l << fnRet(result);

		*l << hasValue;
		// We had a value:
		if (Value(k).isObject()) {
			// If it is an object, we can just copy a pointer.
			*l << mov(result, ptrRel(ptrA));
		} else {
			// It was a class. Call the appropriate constructor in Value.
			SimplePart *ctorName = new (this) SimplePart(Type::CTOR);
			ctorName->params->push(maybe.asRef());
			ctorName->params->push(Value(k).asRef());
			Function *ctor = as<Function>(maybe.type->find(ctorName, Scope()));
			if (!ctor)
				throw new (this) InternalError(S("Failed to find the constructor in Maybe<T>."));

			*l << lea(ptrC, result);
			*l << fnParam(ptr, ptrC);
			*l << fnParam(ptr, ptrA);
			*l << fnCall(ctor->ref(), true);
		}
		*l << activate(result);
		*l << fnRet(result);

		add(dynamicFunction(engine, maybe, S("at"), valList(engine, 2, thisPtr(this), Value(k, true)), l));
	}

	void SetType::notifyAdded(NameSet *to, Named *added) {
		if (to != k)
			return;

		if (watchFor & watchSerialization) {
			if (SerializeInfo *info = serializeInfo(k)) {
				addSerialization(info);
				watchFor &= ~watchSerialization;
			}
		}

		if (!watchFor)
			k->watchRemove(this);
	}

	void SetType::addSerialization(SerializeInfo *info) {
		Function *ctor = readCtor(info);
		add(ctor);

		SerializedTuples *type = new (this) SerializedTuples(this, pointer(ctor));
		type->add(k);
		add(serializedTypeFn(type));
		add(writeFn(type, info));
		add(serializedReadFn(this));
	}

	Function *SetType::writeFn(SerializedType *type, SerializeInfo *info) {
		using namespace code;

		Value me = thisPtr(this);
		Value objStream(StormInfo<ObjOStream>::type(engine));
		Value natType(StormInfo<Nat>::type(engine));

		Function *startObjFn = findStormMemberFn(objStream, S("startClass"),
											Value(StormInfo<SerializedType>::type(engine)),
											Value(StormInfo<Object>::type(engine)));
		Function *endObjFn = findStormMemberFn(objStream, S("end"));
		Function *natWriteFn = findStormMemberFn(natType, S("write"), objStream);
		Function *countFn = findStormMemberFn(me, S("count"));
		Function *beginFn = findStormMemberFn(me, S("begin"));
		Function *endFn = findStormMemberFn(me, S("end"));

		Value iter = beginFn->result.asRef(false);
		Value refIter = iter.asRef();
		Function *iterNext = findStormMemberFn(refIter, S("++*"));
		Function *iterEq = findStormMemberFn(refIter, S("=="), refIter);
		Function *iterVal = findStormMemberFn(refIter, S("v"));

		Listing *l = new (this) Listing(true, engine.voidDesc());
		code::Var meVar = l->createParam(me.desc(engine));
		code::Var streamVar = l->createParam(objStream.desc(engine));
		// Note: We know that we don't need destructors for the iterators...
		code::Var iterVar = l->createVar(l->root(), iter.size());
		code::Var endVar = l->createVar(l->root(), iter.size());

		code::Label lblEnd = l->label();
		code::Label lblLoop = l->label();
		code::Label lblLoopEnd = l->label();

		*l << prolog();

		// Call "start".
		*l << fnParam(objStream.desc(engine), streamVar);
		*l << fnParam(engine.ptrDesc(), objPtr(type));
		*l << fnParam(me.desc(engine), meVar);
		*l << fnCall(startObjFn->ref(), true, byteDesc(engine), al);

		// See if we need to serialize ourselves.
		*l << cmp(al, byteConst(0));
		*l << jmp(lblEnd, ifEqual);

		// Find and write number of elements.
		*l << fnParam(me.desc(engine), meVar);
		*l << fnCall(countFn->ref(), true, natType.desc(engine), eax);
		*l << fnParam(natType.desc(engine), eax);
		*l << fnParam(objStream.desc(engine), streamVar);
		*l << fnCall(natWriteFn->ref(), true);

		// Extract 'begin' and 'end'.
		*l << fnParam(me.desc(engine), meVar);
		*l << fnCall(beginFn->ref(), true, iter.desc(engine), iterVar);
		*l << fnParam(me.desc(engine), meVar);
		*l << fnCall(endFn->ref(), true, iter.desc(engine), endVar);

		// Compare 'begin' and 'end'.
		*l << lblLoop;
		*l << lea(ptrA, endVar);
		*l << lea(ptrC, iterVar);
		*l << fnParam(refIter.desc(engine), ptrA);
		*l << fnParam(refIter.desc(engine), ptrC);
		*l << fnCall(iterEq->ref(), true, byteDesc(engine), al);

		*l << cmp(al, byteConst(0));
		*l << jmp(lblLoopEnd, ifNotEqual);

		// Output.
		*l << lea(ptrA, iterVar);
		*l << fnParam(refIter.desc(engine), ptrA);
		*l << fnCall(iterVal->ref(), true, engine.ptrDesc(), ptrA);

		Value keyT(k);
		if (keyT.isObject())
			*l << mov(ptrA, ptrRel(ptrA, Offset()));
		if (!keyT.isObject() && !info->write->params->at(0).ref)
			*l << fnParamRef(keyT.desc(engine), ptrA);
		else
			*l << fnParam(engine.ptrDesc(), ptrA);
		*l << fnParam(objStream.desc(engine), streamVar);
		*l << fnCall(info->write->ref(), true);

		// Increment the iterator.
		*l << lea(ptrA, iterVar);
		*l << fnParam(refIter.desc(engine), ptrA);
		*l << fnCall(iterNext->ref(), true);

		*l << jmp(lblLoop);
		*l << lblLoopEnd;

		// Call "end".
		*l << fnParam(objStream.desc(engine), streamVar);
		*l << fnCall(endObjFn->ref(), true);

		*l << lblEnd;
		*l << fnRet();

		Array<Value> *params = new (this) Array<Value>();
		params->reserve(2);
		*params << me << objStream;
		Function *fn = new (this) Function(Value(), new (this) Str(S("write")), params);
		fn->setCode(new (this) DynamicCode(l));
		return fn;
	}

	Function *SetType::readCtor(SerializeInfo *info) {
		using namespace code;

		Value me = thisPtr(this);
		Value objStream(StormInfo<ObjIStream>::type(engine));
		Value natType(StormInfo<Nat>::type(engine));
		Value kType(k);

		Function *initFn = findStormMemberFn(me, Type::CTOR);
		Function *endFn = findStormMemberFn(objStream, S("end"));
		Function *natReadFn = findStormFn(natType, S("read"), objStream);
		Function *putFn = findStormMemberFn(me, S("put"), Value(k, true));
		Function *checkArrayFn = findStormMemberFn(objStream, S("checkArrayAlloc"), natType, natType);

		Listing *l = new (this) Listing(true, engine.voidDesc());
		code::Var meVar = l->createParam(me.desc(engine));
		code::Var streamVar = l->createParam(objStream.desc(engine));
		code::Var count = l->createVar(l->root(), Size::sInt);
		code::Var curr = l->createVar(l->root(), Size::sInt);

		code::Label lblLoop = l->label();
		code::Label lblLoopEnd = l->label();

		*l << prolog();

		// Call the default constructor.
		*l << fnParam(me.desc(engine), meVar);
		*l << fnCall(initFn->ref(), true);

		// Find number of elements.
		*l << fnParam(objStream.desc(engine), streamVar);
		*l << fnCall(natReadFn->ref(), false, natType.desc(engine), count);

		// Check size of array.
		*l << fnParam(objStream.desc(engine), streamVar);
		*l << fnParam(natType.desc(engine), natConst(kType.size()));
		*l << fnParam(natType.desc(engine), count);
		*l << fnCall(checkArrayFn->ref(), true);

		// Read each element.
		*l << lblLoop;
		*l << cmp(curr, count);
		*l << jmp(lblLoopEnd, ifAboveEqual);

		code::Block sub = l->createBlock(l->root());
		*l << code::begin(sub);

		// Read key.
		code::Var keyVar = l->createVar(sub, kType.size(), kType.destructor(), code::freeDef | code::freeInactive);
		*l << fnParam(objStream.desc(engine), streamVar);
		*l << fnCall(info->read->ref(), false, kType.desc(engine), keyVar);
		*l << code::activate(keyVar);

		// Call 'put'.
		*l << lea(ptrA, keyVar);
		*l << fnParam(me.desc(engine), meVar);
		*l << fnParam(engine.ptrDesc(), ptrA);
		*l << fnCall(putFn->ref(), true);

		// Repeat!
		*l << code::add(curr, natConst(1));
		*l << jmp(lblLoop);

		// Call 'end'.
		*l << lblLoopEnd;
		*l << fnParam(objStream.desc(engine), streamVar);
		*l << fnCall(endFn->ref(), true);

		*l << fnRet();

		Array<Value> *params = new (this) Array<Value>();
		params->reserve(2);
		*params << me << objStream;
		Function *fn = new (this) Function(Value(), new (this) Str(Type::CTOR), params);
		fn->setCode(new (this) DynamicCode(l));
		fn->visibility = typePrivate(engine);
		return fn;
	}


	/**
	 * Iterator.
	 */

	static void CODECALL copyIterator(void *to, const SetBase::Iter *from) {
		new (Place(to)) SetBase::Iter(*from);
	}

	static bool CODECALL iteratorEq(SetBase::Iter &a, SetBase::Iter &b) {
		return a == b;
	}

	static bool CODECALL iteratorNeq(SetBase::Iter &a, SetBase::Iter &b) {
		return a != b;
	}

	static void *CODECALL iteratorGet(const SetBase::Iter &v) {
		return v.rawVal();
	}

	SetIterType::SetIterType(Type *k)
		: Type(new (k) Str(S("Iter")), new (k) Array<Value>(), typeValue),
		  k(k) {

		setSuper(SetBase::Iter::stormType(engine));
	}

	Bool SetIterType::loadAll() {
		Engine &e = engine;

		// TODO: Return a value for the key!

		Value key = Value(k);
		Value keyRef = key.asRef();
		Value vBool = Value(StormInfo<Bool>::type(e));

		Value v = Value(this);
		Value r = v.asRef();

		Array<Value> *ref = valList(e, 1, r);
		Array<Value> *refref = valList(e, 2, r, r);

		add(nativeFunction(e, Value(), Type::CTOR, refref, address(&copyIterator))->makePure());
		add(nativeFunction(e, vBool, S("=="), refref, address(&iteratorEq))->makePure());
		add(nativeFunction(e, vBool, S("!="), refref, address(&iteratorNeq))->makePure());
		add(nativeFunction(e, r, S("++*"), ref, address(&SetBase::Iter::preIncRaw)));
		add(nativeFunction(e, v, S("*++"), ref, address(&SetBase::Iter::postIncRaw)));
		add(nativeFunction(e, keyRef, S("k"), ref, address(&iteratorGet))->makePure());
		add(nativeFunction(e, keyRef, S("v"), ref, address(&iteratorGet))->makePure());

		return Type::loadAll();
	}

}
