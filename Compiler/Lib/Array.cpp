#include "stdafx.h"
#include "Array.h"
#include "Engine.h"
#include "Exception.h"
#include "Fn.h"
#include "Core/Str.h"
#include "Serialization.h"

namespace storm {

	Type *createArray(Str *name, ValueArray *params) {
		if (params->count() != 1)
			return null;

		Value param = params->at(0);
		if (param.ref)
			return null;

		return new (params) ArrayType(name, param.type);
	}

	Bool isArray(Value v) {
		return as<ArrayType>(v.type) != null;
	}

	Value unwrapArray(Value v) {
		if (ArrayType *t = as<ArrayType>(v.type))
			return t->param();
		else
			return v;
	}

	Value wrapArray(Value v) {
		if (!v.type)
			return v;
		if (v.ref)
			return v;

		Engine &e = v.type->engine;
		TemplateList *l = e.cppTemplate(ArrayId);
		NameSet *to = l->addTo();
		assert(to, L"Too early to use 'wrapArray'.");
		Type *found = as<Type>(to->find(S("Array"), v, Scope()));
		if (!found)
			throw new (v.type) InternalError(S("Can not find the array type!"));
		return Value(found);
	}

	static void CODECALL createArrayRaw(void *mem) {
		ArrayType *t = (ArrayType *)runtime::typeOf((RootObject *)mem);
		ArrayBase *o = new (Place(mem)) ArrayBase(t->param().type->handle());
		runtime::setVTable(o);
	}

	static void CODECALL copyArray(void *mem, ArrayBase *from) {
		ArrayBase *o = new (Place(mem)) ArrayBase(*from);
		runtime::setVTable(o);
	}

	static void CODECALL createArrayClassCount(void *mem, Nat count, const RootObject *data) {
		ArrayType *t = (ArrayType *)runtime::typeOf((RootObject *)mem);
		ArrayBase *o = new (Place(mem)) ArrayBase(t->param().type->handle(), count, &data);
		runtime::setVTable(o);
	}

	static void CODECALL createArrayValCount(void *mem, Nat count, const void *data) {
		ArrayType *t = (ArrayType *)runtime::typeOf((RootObject *)mem);
		ArrayBase *o = new (Place(mem)) ArrayBase(t->param().type->handle(), count, data);
		runtime::setVTable(o);
	}

	static ArrayBase *CODECALL pushClass(ArrayBase *to, const void *src) {
		to->pushRaw(&src);
		return to;
	}

	static void CODECALL insertClass(ArrayBase *to, Nat pos, const void *src) {
		to->insertRaw(pos, &src);
	}

	static ArrayBase *CODECALL pushValue(ArrayBase *to, const void *src) {
		to->pushRaw(src);
		return to;
	}

	static ArrayBase *CODECALL sortedRaw(ArrayBase *src) {
		Type *t = runtime::typeOf(src);
		ArrayBase *copy = (ArrayBase *)runtime::allocObject(sizeof(ArrayBase), t);
		copyArray(copy, src);
		copy->sortRaw();
		return copy;
	}

	static ArrayBase *CODECALL sortedRawPred(ArrayBase *src, FnBase *compare) {
		Type *t = runtime::typeOf(src);
		ArrayBase *copy = (ArrayBase *)runtime::allocObject(sizeof(ArrayBase), t);
		copyArray(copy, src);
		copy->sortRawPred(compare);
		return copy;
	}

	ArrayType::ArrayType(Str *name, Type *contents) : Type(name, typeClass), contents(contents), watchFor(0) {
		if (engine.has(bootTemplates))
			lateInit();

		setSuper(ArrayBase::stormType(engine));
		// Avoid problems while loading Array<Value>, as Array<Value> is required to compute the
		// GcType of Array<Value>...
		useSuperGcType();
	}

	void ArrayType::lateInit() {
		if (!params)
			params = new (engine) Array<Value>();
		if (params->count() < 1)
			params->push(Value(contents));

		Type::lateInit();
	}

	Value ArrayType::param() const {
		return Value(contents);
	}

	Bool ArrayType::loadAll() {
		Type *iter = new (this) ArrayIterType(contents);
		add(iter);

		if (param().isObject())
			loadClassFns();
		else
			loadValueFns();

		// Shared functions.
		Engine &e = engine;
		Value t = thisPtr(this);
		Value ref = param().asRef();
		Value natType = Value(StormInfo<Nat>::type(e));

		add(nativeFunction(e, Value(), Type::CTOR, valList(e, 2, t, t), address(&copyArray))->makePure());
		add(nativeFunction(e, ref, S("[]"), valList(e, 2, t, natType), address(&ArrayBase::getRaw))->makePure());
		add(nativeFunction(e, ref, S("last"), valList(e, 1, t), address(&ArrayBase::lastRaw))->makePure());
		add(nativeFunction(e, ref, S("first"), valList(e, 1, t), address(&ArrayBase::firstRaw))->makePure());
		add(nativeFunction(e, ref, S("random"), valList(e, 1, t), address(&ArrayBase::randomRaw)));
		add(nativeFunction(e, t, S("append"), valList(e, 2, t, t), address(&ArrayBase::appendRaw)));
		add(nativeFunction(e, Value(iter), S("begin"), valList(e, 1, t), address(&ArrayBase::beginRaw))->makePure());
		add(nativeFunction(e, Value(iter), S("end"), valList(e, 1, t), address(&ArrayBase::endRaw))->makePure());

		// Sort with operator <.
		if (param().type->handle().lessFn) {
			addSort();
		} else {
			watchFor |= watchLess;
		}


		// Sort with provided predicate function.
		{
			Array<Value> *pParams = new (e) Array<Value>();
			*pParams << Value(StormInfo<Bool>::type(e));
			*pParams << param() << param();
			Value predicate = Value(fnType(pParams));
			add(nativeFunction(e, Value(), S("sort"), valList(e, 2, t, predicate), address(&ArrayBase::sortRawPred)));
			add(nativeFunction(e, t, S("sorted"), valList(e, 2, t, predicate), address(&sortedRawPred))->makePure());
		}

		if (!param().type->isA(StormInfo<TObject>::type(engine))) {
			if (SerializeInfo *info = serializeInfo(param().type)) {
				addSerialization(info);
			} else {
				watchFor |= watchSerialization;
			}
		}

		if (watchFor)
			param().type->watchAdd(this);

		return Type::loadAll();
	}

	void ArrayType::loadClassFns() {
		Engine &e = engine;
		Value t = thisPtr(this);
		Value natType = Value(StormInfo<Nat>::type(e));

		add(nativeFunction(e, Value(), Type::CTOR, valList(e, 1, t), address(&createArrayRaw))->makePure());
		add(nativeFunction(e, Value(), Type::CTOR, valList(e, 3, t, natType, param()), address(&createArrayClassCount))->makePure());
		add(nativeFunction(e, t, S("<<"), valList(e, 2, t, param()), address(&pushClass)));
		add(nativeFunction(e, t, S("push"), valList(e, 2, t, param()), address(&pushClass)));
		add(nativeFunction(e, Value(), S("insert"), valList(e, 3, t, natType, param()), address(&insertClass)));
	}

	void ArrayType::loadValueFns() {
		Engine &e = engine;
		Value t = thisPtr(this);
		Value ref = param().asRef(true);
		Value natType = Value(StormInfo<Nat>::type(e));

		add(nativeFunction(e, Value(), Type::CTOR, valList(e, 1, t), address(&createArrayRaw))->makePure());
		add(nativeFunction(e, Value(), Type::CTOR, valList(e, 3, t, natType, ref), address(&createArrayValCount))->makePure());
		add(nativeFunction(e, t, S("<<"), valList(e, 2, t, ref), address(&pushValue)));
		add(nativeFunction(e, t, S("push"), valList(e, 2, t, ref), address(&pushValue)));
		add(nativeFunction(e, Value(), S("insert"), valList(e, 3, t, natType, ref), address(&ArrayBase::insertRaw)));
	}

	void ArrayType::notifyAdded(NameSet *to, Named *added) {
		if (to != param().type)
			return;

		Function *fn = as<Function>(added);
		if (!fn)
			return;

		if (*fn->name == S("<") &&
			fn->result == Value(StormInfo<Bool>::type(engine)) &&
			fn->params->count() == 2 &&
			fn->params->at(0).type == to &&
			fn->params->at(1).type == to) {

			watchFor &= ~watchLess;
			addSort();
		} else if (SerializeInfo *info = serializeInfo(param().type)) {
			if (watchFor & watchSerialization) {
				watchFor &= ~watchSerialization;
				addSerialization(info);
			}
		}


		if (watchFor == watchNone)
			to->watchRemove(this);
	}

	void ArrayType::addSort() {
		Engine &e = engine;
		Array<Value> *params = valList(e, 1, thisPtr(this));

		// Sort using <.
		add(nativeFunction(e, Value(), S("sort"), params, address(&ArrayBase::sortRaw)));
		add(nativeFunction(e, Value(this), S("sorted"), params, address(&sortedRaw))->makePure());

		// TODO: Would be nice to have < for comparison as well!
	}

	void ArrayType::addSerialization(SerializeInfo *info) {
		Function *ctor = readCtor(info);
		add(ctor);

		SerializedTuples *type = new (this) SerializedTuples(this, pointer(ctor));
		type->add(param().type);
		add(serializedTypeFn(type));

		// Add the 'write' function.
		add(writeFn(type, info));

		// Add the 'read' function.
		add(serializedReadFn(this));
	}

	Function *ArrayType::writeFn(SerializedType *type, SerializeInfo *info) {
		using namespace code;

		Value me = thisPtr(this);
		Value objStream(StormInfo<ObjOStream>::type(engine));

		Function *startFn = findStormMemberFn(objStream, S("startClass"),
											Value(StormInfo<SerializedType>::type(engine)),
											Value(StormInfo<Object>::type(engine)));
		Function *endFn = findStormMemberFn(objStream, S("end"));
		Function *natWriteFn = findStormMemberFn(Value(StormInfo<Nat>::type(engine)), S("write"), objStream);
		Function *countFn = findStormMemberFn(me, S("count"));
		Function *atFn = findStormMemberFn(me, S("[]"), Value(StormInfo<Nat>::type(engine)));

		Listing *l = new (this) Listing(true, engine.voidDesc());
		code::Var meVar = l->createParam(me.desc(engine));
		code::Var streamVar = l->createParam(objStream.desc(engine));
		code::Var count = l->createVar(l->root(), Size::sInt);
		code::Var curr = l->createVar(l->root(), Size::sInt);

		TypeDesc *natDesc = code::intDesc(engine); // int === nat in this context.
		TypeDesc *ptrDesc = code::ptrDesc(engine);

		code::Label lblEnd = l->label();
		code::Label lblLoop = l->label();
		code::Label lblLoopEnd = l->label();

		*l << prolog();

		// Call "start".
		*l << fnParam(objStream.desc(engine), streamVar);
		*l << fnParam(engine.ptrDesc(), objPtr(type));
		*l << fnParam(me.desc(engine), meVar);
		*l << fnCall(startFn->ref(), true, byteDesc(engine), al);

		// See if we need to serialize ourselves.
		*l << cmp(al, byteConst(0));
		*l << jmp(lblEnd, ifEqual);

		// Find number of elements.
		*l << fnParam(me.desc(engine), meVar);
		*l << fnCall(countFn->ref(), true, natDesc, count);

		// Write number of elements.
		*l << fnParam(natDesc, count);
		*l << fnParam(objStream.desc(engine), streamVar);
		*l << fnCall(natWriteFn->ref(), true);

		// Serialize each element.
		*l << lblLoop;
		*l << cmp(curr, count);
		*l << jmp(lblLoopEnd, ifAboveEqual);

		// Get the element.
		*l << fnParam(me.desc(engine), meVar);
		*l << fnParam(natDesc, curr);
		*l << fnCall(atFn->ref(), true, ptrDesc, ptrA);

		// If it is a pointer, we need to read the actual pointer.
		if (param().isObject())
			*l << mov(ptrA, ptrRel(ptrA, Offset()));

		// Call 'write'. If the object is a value, and does not take a reference parameter as the
		// 'this' parameter (some primitive types do that), we need to dereference it.
		if (!param().isObject() && !info->write->params->at(0).ref)
			*l << fnParamRef(param().desc(engine), ptrA);
		else
			*l << fnParam(ptrDesc, ptrA);
		*l << fnParam(objStream.desc(engine), streamVar);
		*l << fnCall(info->write->ref(), true);

		*l << code::add(curr, natConst(1));
		*l << jmp(lblLoop);

		*l << lblLoopEnd;

		// Call "end".
		*l << fnParam(objStream.desc(engine), streamVar);
		*l << fnCall(endFn->ref(), true);

		*l << lblEnd;
		*l << fnRet();

		Array<Value> *params = new (this) Array<Value>();
		params->reserve(2);
		*params << me << objStream;
		Function *fn = new (this) Function(Value(), new (this) Str(S("write")), params);
		fn->setCode(new (this) DynamicCode(l));
		return fn;
	}

	Function *ArrayType::readCtor(SerializeInfo *info) {
		using namespace code;

		Value me = thisPtr(this);
		Value param = this->param();
		Value objStream(StormInfo<ObjIStream>::type(engine));

		Function *initFn = findStormMemberFn(me, Type::CTOR);
		Function *endFn = findStormMemberFn(objStream, S("end"));
		Function *natReadFn = findStormFn(Value(StormInfo<Nat>::type(engine)), S("read"), objStream);
		Function *reserveFn = findStormMemberFn(me, S("reserve"), Value(StormInfo<Nat>::type(engine)));
		Function *pushFn = findStormMemberFn(me, S("<<"), param.asRef(true));

		Listing *l = new (this) Listing(true, engine.voidDesc());
		code::Var meVar = l->createParam(me.desc(engine));
		code::Var streamVar = l->createParam(objStream.desc(engine));
		code::Var count = l->createVar(l->root(), Size::sInt);
		code::Var curr = l->createVar(l->root(), Size::sInt);

		TypeDesc *natDesc = code::intDesc(engine); // int === nat in this context.
		TypeDesc *ptrDesc = code::ptrDesc(engine);

		code::Label lblLoop = l->label();
		code::Label lblLoopEnd = l->label();

		*l << prolog();

		// Call the default constructor.
		*l << fnParam(me.desc(engine), meVar);
		*l << fnCall(initFn->ref(), true);

		// Find number of elements.
		*l << fnParam(objStream.desc(engine), streamVar);
		*l << fnCall(natReadFn->ref(), false, natDesc, count);

		// Reserve elements.
		*l << fnParam(me.desc(engine), meVar);
		*l << fnParam(natDesc, count);
		*l << fnCall(reserveFn->ref(), true);

		// Read each element.
		*l << lblLoop;
		*l << cmp(curr, count);
		*l << jmp(lblLoopEnd, ifAboveEqual);

		if (param.isObject()) {
			// Get the element.
			*l << fnParam(objStream.desc(engine), streamVar);
			*l << fnCall(info->read->ref(), false, param.desc(engine), ptrA);

			// Store it.
			*l << fnParam(me.desc(engine), meVar);
			*l << fnParam(ptrDesc, ptrA);
			*l << fnCall(pushFn->ref(), true, ptrDesc, ptrA); // It returns a ptr to itself, but we don't need that.
		} else {
			code::Block sub = l->createBlock(l->root());
			code::Var tmp = l->createVar(sub, param.size(), param.destructor(), code::freeDef | code::freeInactive);

			*l << code::begin(sub);

			// Get the element.
			*l << fnParam(objStream.desc(engine), streamVar);
			*l << fnCall(info->read->ref(), false, param.desc(engine), tmp);

			// Make sure it is properly destroyed if we get an exception.
			*l << code::activate(tmp);

			// Write it to the array by calling 'push'.
			*l << lea(ptrA, tmp);
			*l << fnParam(me.desc(engine), meVar);
			*l << fnParam(ptrDesc, ptrA);
			*l << fnCall(pushFn->ref(), true, ptrDesc, ptrA); // It returns a ptr to itself, but we don't need that.

			// Now, we're done with 'tmp'.
			*l << code::end(sub);
		}

		// Repeat!
		*l << code::add(curr, natConst(1));
		*l << jmp(lblLoop);

		// Call "end".
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

	static void CODECALL copyIterator(void *to, const ArrayBase::Iter *from) {
		new (Place(to)) ArrayBase::Iter(*from);
	}

	static bool CODECALL iteratorEq(ArrayBase::Iter &a, ArrayBase::Iter &b) {
		return a == b;
	}

	static bool CODECALL iteratorNeq(ArrayBase::Iter &a, ArrayBase::Iter &b) {
		return a != b;
	}

	static void *CODECALL iteratorGet(const ArrayBase::Iter &v) {
		return v.getRaw();
	}

	static Nat CODECALL iteratorGetKey(const ArrayBase::Iter &v) {
		return v.getIndex();
	}

	ArrayIterType::ArrayIterType(Type *param)
		: Type(new (param) Str(S("Iter")), new (param) Array<Value>(), typeValue),
		  contents(param) {

		setSuper(ArrayBase::Iter::stormType(engine));
	}

	Bool ArrayIterType::loadAll() {
		Engine &e = engine;
		Value v(this, false);
		Value r(this, true);
		Value param(contents, true);

		Array<Value> *ref = valList(e, 1, r);
		Array<Value> *refref = valList(e, 2, r, r);

		Value vBool = Value(StormInfo<Bool>::type(e));
		Value vNat = Value(StormInfo<Nat>::type(e));
		add(nativeFunction(e, Value(), Type::CTOR, refref, address(&copyIterator))->makePure());
		add(nativeFunction(e, vBool, S("=="), refref, address(&iteratorEq))->makePure());
		add(nativeFunction(e, vBool, S("!="), refref, address(&iteratorNeq))->makePure());
		add(nativeFunction(e, r, S("++*"), ref, address(&ArrayBase::Iter::preIncRaw)));
		add(nativeFunction(e, v, S("*++"), ref, address(&ArrayBase::Iter::postIncRaw)));
		add(nativeFunction(e, vNat, S("k"), ref, address(&iteratorGetKey))->makePure());
		add(nativeFunction(e, param, S("v"), ref, address(&iteratorGet))->makePure());

		return Type::loadAll();
	}

}
