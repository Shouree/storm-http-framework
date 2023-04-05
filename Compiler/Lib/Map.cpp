#include "stdafx.h"
#include "Map.h"
#include "Fn.h"
#include "Core/Map.h"
#include "Core/Str.h"
#include "Compiler/Engine.h"
#include "Serialization.h"

namespace storm {

	Type *createMap(Str *name, ValueArray *params) {
		if (params->count() != 2)
			return null;

		Value k = params->at(0);
		Value v = params->at(1);
		if (k.ref || v.ref)
			return null;

		return new (params) MapType(name, k.type, v.type, false);
	}

	Type *createRefMap(Str *name, ValueArray *params) {
		if (params->count() != 2)
			return null;
		Value k = params->at(0);
		Value v = params->at(1);
		if (k.ref || v.ref)
			return null;

		if (k.isValue())
			return null;

		return new (params) MapType(name, k.type, v.type, true);
	}

	MapType::MapType(Str *name, Type *k, Type *v, Bool ref) : Type(name, typeClass), k(k), v(v), refKeys(ref) {
		if (engine.has(bootTemplates))
			lateInit();

		setSuper(MapBase::stormType(engine));
	}

	void MapType::lateInit() {
		if (!params)
			params = new (engine) Array<Value>();
		if (params->count() != 2) {
			params->clear();
			params->push(Value(k));
			params->push(Value(v));
		}
	}

	void CODECALL MapType::createClass(void *mem) {
		MapType *t = (MapType *)runtime::typeOf((RootObject *)mem);
		MapBase *o = new (Place(mem)) MapBase(t->k->handle(), t->v->handle());
		runtime::setVTable(o);
	}

	void CODECALL MapType::createRefClass(void *mem) {
		MapType *t = (MapType *)runtime::typeOf((RootObject *)mem);
		MapBase *o = new (Place(mem)) MapBase(t->engine.refObjHandle(), t->v->handle());
		runtime::setVTable(o);
	}

	void CODECALL MapType::copyClass(void *mem, MapBase *src) {
		MapBase *o = new (Place(mem)) MapBase(*src);
		runtime::setVTable(o);
	}

	Bool MapType::loadAll() {
		Engine &e = engine;

		Value t = thisPtr(this);
		Value key = Value(k);
		Value val = Value(v);
		Value keyRef = key.asRef();
		Value valRef = val.asRef();
		Value iter = Value(new (this) MapIterType(k, v));
		Value boolT = Value(StormInfo<Bool>::type(e));

		Array<Value> *thisKey = valList(e, 2, t, keyRef);

		add(iter.type);
		if (refKeys) {
			add(nativeFunction(e, Value(), Type::CTOR, valList(e, 1, t), address(&MapType::createRefClass))->makePure());
		} else {
			add(nativeFunction(e, Value(), Type::CTOR, valList(e, 1, t), address(&MapType::createClass))->makePure());
		}
		add(nativeFunction(e, Value(), Type::CTOR, valList(e, 2, t, t), address(&MapType::copyClass))->makePure());
		add(nativeFunction(e, boolT, S("put"), valList(e, 3, t, keyRef, valRef), address(&MapBase::putRaw)));
		add(nativeFunction(e, boolT, S("has"), thisKey, address(&MapBase::hasRaw))->makePure());
		add(nativeFunction(e, valRef, S("get"), thisKey, address(&MapBase::getRaw))->makePure());
		add(nativeFunction(e, valRef, S("get"), valList(e, 3, t, keyRef, valRef), address(&MapBase::getRawDef)));
		add(nativeFunction(e, boolT, S("remove"), thisKey, address(&MapBase::removeRaw)));
		add(nativeFunction(e, iter, S("begin"), valList(e, 1, t), address(&MapBase::beginRaw))->makePure());
		add(nativeFunction(e, iter, S("end"), valList(e, 1, t), address(&MapBase::endRaw))->makePure());
		add(nativeFunction(e, iter, S("find"), thisKey, address(&MapBase::findRaw))->makePure());

		Type *tObj = StormInfo<TObject>::type(e);

		if (!v->isA(tObj)) {
			// We currently don't support TObjects as values, that would break thread safety.
			addAccess();
		}

		if (!k->isA(tObj) && !v->isA(tObj)) {
			SerializeInfo *kInfo = serializeInfo(k);
			SerializeInfo *vInfo = serializeInfo(v);

			if (!kInfo)
				watchFor |= watchKeySerialization;
			if (!vInfo)
				watchFor |= watchValueSerialization;

			if (kInfo && vInfo)
				addSerialization(kInfo, vInfo);
		}

		if (watchFor & watchKeyMask)
			k->watchAdd(this);
		if (watchFor & watchValueMask)
			v->watchAdd(this);

		return Type::loadAll();
	}

	void MapType::addAccess() {
		Function *ctor = v->defaultCtor();
		// The 'at' member can only be implemented if 'v' has a default constructor.
		if (!ctor) {
			watchFor |= watchKeyDefaultCtor;
			return;
		}

		watchFor &= ~watchKeyDefaultCtor;

		using namespace code;
		TypeDesc *ptr = engine.ptrDesc();
		Listing *l = new (this) Listing(true, ptr);
		Var me = l->createParam(ptr);
		Var key = l->createParam(ptr);

		*l << prolog();

		if (Value(v).isValue()) {
			*l << fnParam(ptr, me);
			*l << fnParam(ptr, key);
			*l << fnParam(ptr, ctor->ref());
			*l << fnCall(engine.ref(builtin::mapAtValue), true, ptr, ptrA);
		} else {
			*l << fnParam(ptr, me);
			*l << fnParam(ptr, key);
			*l << fnParam(ptr, v->typeRef());
			*l << fnParam(ptr, ctor->ref());
			*l << fnCall(engine.ref(builtin::mapAtClass), true, ptr, ptrA);
		}

		*l << fnRet(ptrA);

		add(dynamicFunction(engine, Value(v, true), S("[]"), valList(engine, 2, thisPtr(this), Value(k, true)), l));
	}

	void MapType::notifyAdded(NameSet *to, Named *added) {
		SerializeInfo *kInfo = null;
		SerializeInfo *vInfo = null;

		if (to == k) {
			if (watchFor & watchKeyDefaultCtor) {
				addAccess();
			}

			if (watchFor & watchKeySerialization) {
				if (kInfo = serializeInfo(k))
					watchFor &= ~watchKeySerialization;
			}

			if ((watchFor & watchKeyMask) == 0)
				k->watchRemove(this);
		}

		if (to == v) {
			if (watchFor & watchValueSerialization) {
				if (vInfo = serializeInfo(v))
					watchFor &= ~watchValueSerialization;
			}

			if ((watchFor & watchValueMask) == 0)
				v->watchRemove(this);
		}

		if ((kInfo || vInfo) && (watchFor & (watchKeySerialization | watchValueSerialization)) == 0) {
			if (!kInfo)
				kInfo = serializeInfo(k);
			if (!vInfo)
				vInfo = serializeInfo(v);

			// Should always be true...
			if (kInfo && vInfo)
				addSerialization(kInfo, vInfo);
		}
	}

	void MapType::addSerialization(SerializeInfo *kInfo, SerializeInfo *vInfo) {
		Function *ctor = readCtor(kInfo, vInfo);
		add(ctor);

		SerializedTuples *type = new (this) SerializedTuples(this, pointer(ctor));
		type->add(k);
		type->add(v);
		add(serializedTypeFn(type));
		add(writeFn(type, kInfo, vInfo));
		add(serializedReadFn(this));
	}

	static void doWrite(code::Listing *l, code::Var iter, code::Var stream, Type *type, Function *retrieve, SerializeInfo *info) {
		Engine &e = l->engine();

		using namespace code;
		*l << lea(ptrA, iter);
		*l << fnParam(e.ptrDesc(), ptrA);
		*l << fnCall(retrieve->ref(), true, e.ptrDesc(), ptrA);

		Value t(type);
		if (t.isObject())
			*l << mov(ptrA, ptrRel(ptrA, Offset()));
		if (!t.isObject() && !info->write->params->at(0).ref)
			*l << fnParamRef(t.desc(e), ptrA);
		else
			*l << fnParam(e.ptrDesc(), ptrA);
		*l << fnParam(e.ptrDesc(), stream);
		*l << fnCall(info->write->ref(), true);
	}

	Function *MapType::writeFn(SerializedType *type, SerializeInfo *kInfo, SerializeInfo *vInfo) {
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
		Function *iterKey = findStormMemberFn(refIter, S("k"));
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
		doWrite(l, iterVar, streamVar, k, iterKey, kInfo);
		doWrite(l, iterVar, streamVar, v, iterVal, vInfo);

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

	Function *MapType::readCtor(SerializeInfo *kInfo, SerializeInfo *vInfo) {
		using namespace code;

		Value me = thisPtr(this);
		Value objStream(StormInfo<ObjIStream>::type(engine));
		Value natType(StormInfo<Nat>::type(engine));
		Value kType(k);
		Value vType(v);

		Function *initFn = findStormMemberFn(me, Type::CTOR);
		Function *endFn = findStormMemberFn(objStream, S("end"));
		Function *natReadFn = findStormFn(natType, S("read"), objStream);
		Function *putFn = findStormMemberFn(me, S("put"), Value(k, true), Value(v, true));
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

		// Check size of keys array.
		*l << fnParam(objStream.desc(engine), streamVar);
		*l << fnParam(natType.desc(engine), natConst(kType.size()));
		*l << fnParam(natType.desc(engine), count);
		*l << fnCall(checkArrayFn->ref(), true);

		// Check size of values array.
		*l << fnParam(objStream.desc(engine), streamVar);
		*l << fnParam(natType.desc(engine), natConst(vType.size()));
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
		*l << fnCall(kInfo->read->ref(), false, kType.desc(engine), keyVar);
		*l << code::activate(keyVar);

		// Read value.
		code::Var valVar = l->createVar(sub, vType.size(), vType.destructor(), code::freeDef | code::freeInactive);
		*l << fnParam(objStream.desc(engine), streamVar);
		*l << fnCall(vInfo->read->ref(), false, vType.desc(engine), valVar);
		*l << code::activate(valVar);

		// Call 'put'.
		*l << lea(ptrA, keyVar);
		*l << lea(ptrC, valVar);
		*l << fnParam(me.desc(engine), meVar);
		*l << fnParam(engine.ptrDesc(), ptrA);
		*l << fnParam(engine.ptrDesc(), ptrC);
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

	static void copyIterator(void *to, const MapBase::Iter *from) {
		new (Place(to)) MapBase::Iter(*from);
	}

	static bool iteratorEq(MapBase::Iter &a, MapBase::Iter &b) {
		return a == b;
	}

	static bool iteratorNeq(MapBase::Iter &a, MapBase::Iter &b) {
		return a != b;
	}

	MapIterType::MapIterType(Type *k, Type *v)
		: Type(new (k) Str(S("Iter")), new (k) Array<Value>(), typeValue),
		  k(k),
		  v(v) {

		setSuper(MapBase::Iter::stormType(engine));
	}

	Bool MapIterType::loadAll() {
		Engine &e = engine;

		// TODO: Return a value for the key!

		Value key = Value(k);
		Value val = Value(v);
		Value keyRef = key.asRef();
		Value valRef = val.asRef();
		Value vBool = Value(StormInfo<Bool>::type(e));

		Value v = Value(this);
		Value r = v.asRef();

		Array<Value> *ref = valList(e, 1, r);
		Array<Value> *refref = valList(e, 2, r, r);

		add(nativeFunction(e, Value(), Type::CTOR, refref, address(&copyIterator))->makePure());
		add(nativeFunction(e, vBool, S("=="), refref, address(&iteratorEq))->makePure());
		add(nativeFunction(e, vBool, S("!="), refref, address(&iteratorNeq))->makePure());
		add(nativeFunction(e, r, S("++*"), ref, address(&MapBase::Iter::preIncRaw)));
		add(nativeFunction(e, v, S("*++"), ref, address(&MapBase::Iter::postIncRaw)));
		add(nativeFunction(e, keyRef, S("k"), ref, address(&MapBase::Iter::rawKey))->makePure());
		add(nativeFunction(e, valRef, S("v"), ref, address(&MapBase::Iter::rawVal))->makePure());

		return Type::loadAll();
	}

}
