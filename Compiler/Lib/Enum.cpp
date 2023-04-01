#include "stdafx.h"
#include "Enum.h"
#include "Number.h"
#include "Engine.h"
#include "Core/Hash.h"
#include "Function.h"
#include "Fn.h"
#include "Serialization.h"

namespace storm {

	Type *createEnum(Str *name, Size size, GcType *type) {
		return new (name) Enum(name, type, false);
	}

	Type *createBitmaskEnum(Str *name, Size size, GcType *type) {
		return new (name) Enum(name, type, true);
	}

	static GcType *createGcType(Engine &e) {
		return e.gc.allocType(GcType::tArray, null, Size::sInt.current(), 0);
	}

	using namespace code;

	static void enumAdd(InlineParams p) {
		if (p.result->needed()) {
			Operand result = p.result->location(p.state);
			*p.state->l << mov(result, p.param(0));
			*p.state->l << bor(result, p.param(1));
		}
	}

	static void enumSub(InlineParams p) {
		if (p.result->needed()) {
			Operand result = p.result->location(p.state);
			*p.state->l << mov(result, p.param(1));
			*p.state->l << bnot(result);
			*p.state->l << band(result, p.param(0));
		}
	}

	static void enumOverlaps(InlineParams p) {
		if (p.result->needed()) {
			Operand result = p.result->location(p.state);
			*p.state->l << test(p.param(0), p.param(1));
			*p.state->l << setCond(result, ifNotEqual);
		}
	}

	static void enumGet(InlineParams p) {
		if (p.result->needed()) {
			if (!p.result->suggest(p.state, p.param(0)))
				*p.state->l << mov(p.result->location(p.state), p.param(0));
		}
	}

	Enum::Enum(Str *name, Bool bitmask)
		: Type(name, typeValue, Size::sInt, createGcType(name->engine()), null),
		  mask(bitmask) {

		if (engine.has(bootTemplates))
			lateInit();
	}


	Enum::Enum(Str *name, GcType *type, Bool bitmask)
		: Type(name, typeValue, Size::sInt, type, null),
		  mask(bitmask) {

		assert(type->stride == Size::sInt.current(), L"Check the size of your enums!");
		if (engine.has(bootTemplates))
			lateInit();
	}

	void Enum::lateInit() {
		Type::lateInit();

		if (engine.has(bootTemplates) && values == null)
			values = new (this) Array<EnumValue *>();
	}

	Bool Enum::loadAll() {
		// Array<Value> *r = new (this) Array<Value>(1, Value(this, true));
		Array<Value> *rr = new (this) Array<Value>(2, Value(this, true));
		Array<Value> *v = new (this) Array<Value>(1, Value(this, false));
		Array<Value> *vv = new (this) Array<Value>(2, Value(this, false));
		Array<Value> *rv = new (this) Array<Value>(2, Value(this, true));
		rv->at(1) = Value(this);

		Value nat(StormInfo<Nat>::type(engine));
		Value b(StormInfo<Bool>::type(engine));
		Array<Value> *rn = new (this) Array<Value>(2, Value(this, true));
		rn->at(1) = Value(StormInfo<Nat>::type(engine));

		add(inlinedFunction(engine, Value(), Type::CTOR, rr, fnPtr(engine, &numCopyCtor<Nat>))->makePure());
		add(inlinedFunction(engine, Value(), Type::CTOR, rn, fnPtr(engine, &numAssign<Nat>))->makePure());
		add(inlinedFunction(engine, Value(), S("="), rv, fnPtr(engine, &numAssign<Nat>))->makePure());
		add(inlinedFunction(engine, b, S("=="), vv, fnPtr(engine, &numCmp<ifEqual>))->makePure());
		add(inlinedFunction(engine, b, S("!="), vv, fnPtr(engine, &numCmp<ifNotEqual>))->makePure());

		if (mask) {
			add(inlinedFunction(engine, Value(this), S("+"), vv, fnPtr(engine, &enumAdd))->makePure());
			add(inlinedFunction(engine, Value(this), S("-"), vv, fnPtr(engine, &enumSub))->makePure());
			add(inlinedFunction(engine, b, S("&"), vv, fnPtr(engine, &enumOverlaps))->makePure());
			add(inlinedFunction(engine, b, S("has"), vv, fnPtr(engine, &enumOverlaps))->makePure());
		}

		add(inlinedFunction(engine, nat, S("v"), v, fnPtr(engine, &enumGet))->makePure());
		add(nativeFunction(engine, nat, S("hash"), v, address(&intHash))->makePure());

		// Serialization:
		Function *readCtor = createReadCtor();
		add(readCtor);

		SerializedStdType *typeInfo = new (this) SerializedStdType(this, pointer(readCtor));
		typeInfo->add(S("v"), StormInfo<Nat>::type(engine));
		add(createReadFn());
		add(createWriteFn(typeInfo));
		add(serializedTypeFn(typeInfo));

		return Type::loadAll();
	}

	void Enum::add(Named *v) {
		Type::add(v);

		// Keep track of values ourselves as well!
		if (EnumValue *e = as<EnumValue>(v))
			values->push(e);
	}

	static void put(StrBuf *to, const wchar *val, bool &first) {
		if (!first)
			*to << S(" + ");
		first = false;
		*to << val;
	}

	static void put(StrBuf *to, Str *val, bool &first) {
		put(to, val->c_str(), first);
	}

	void Enum::toString(StrBuf *to, Nat v) {
		// Note: this needs to be thread safe...
		bool first = true;
		Nat original = v;

		for (Nat i = 0; i < values->count(); i++) {
			EnumValue *val = values->at(i);

			if (mask) {
				// Note: special case for 'empty' if present.
				if ((val->value & v) || (val->value == original && original == 0)) {
					put(to, val->name, first);
					v = v & ~val->value;
				}
			} else {
				if (val->value == v) {
					put(to, val->name, first);
					v = 0;
				}
			}
		}

		// Any remaining garbage?
		if (v != 0) {
			if (!first)
				*to << S(" + ");
			first = false;
			*to << S("<unknown: ") << v << S(">");
		} else if (mask && first) {
			put(to, S("<none>"), first);
		}
	}

	void Enum::toS(StrBuf *to) const {
		*to << S("enum ") << identifier() << S(" [");
		putVisibility(to);
		*to << S("] {\n");
		{
			Indent z(to);
			for (Nat i = 0; i < values->count(); i++) {
				EnumValue *v = values->at(i);
				if (mask) {
					*to << v->name << S(" = ") << hex(v->value) << S("\n");
				} else {
					*to << v->name << S(" = ") << v->value << S("\n");
				}
			}
		}
		*to << S("}");
	}

	Function *Enum::createWriteFn(SerializedType *type) {
		using namespace code;

		Value me = thisPtr(this);
		Value objStream(StormInfo<ObjOStream>::type(engine));
		Value natType(StormInfo<Nat>::type(engine));

		Function *startFn = findStormMemberFn(objStream, S("startValue"),
											Value(StormInfo<SerializedType>::type(engine)));
		Function *endFn = findStormMemberFn(objStream, S("end"));
		Function *natWriteFn = findStormMemberFn(natType, S("write"), objStream);

		Listing *l = new (this) Listing(true, engine.voidDesc());
		code::Var meVar = l->createParam(me.desc(engine));
		code::Var streamVar = l->createParam(objStream.desc(engine));

		code::Label lblEnd = l->label();

		*l << prolog();

		// Call "start".
		*l << fnParam(objStream.desc(engine), streamVar);
		*l << fnParam(engine.ptrDesc(), objPtr(type));
		*l << fnCall(startFn->ref(), true, byteDesc(engine), al);

		// See if we need to serialize ourselves.
		*l << cmp(al, byteConst(0));
		*l << jmp(lblEnd, ifEqual);

		// Serialize ourselves.
		*l << mov(ptrA, meVar);
		*l << fnParam(natType.desc(engine), intRel(ptrA, Offset()));
		*l << fnParam(objStream.desc(engine), streamVar);
		*l << fnCall(natWriteFn->ref(), true);

		// Call "end".
		*l << fnParam(objStream.desc(engine), streamVar);
		*l << fnCall(endFn->ref(), true);

		// End of function.
		*l << lblEnd;
		*l << fnRet();

		Array<Value> *params = new (this) Array<Value>();
		params->reserve(2);
		*params << me << objStream;
		Function *fn = new (this) Function(Value(), new (this) Str(S("write")), params);
		fn->setCode(new (this) DynamicCode(l));
		return fn;
	}

	Function *Enum::createReadFn() {
		using namespace code;

		Value objStream(StormInfo<ObjIStream>::type(engine));
		Value natType(StormInfo<Nat>::type(engine));

		Function *readValueFn = null;
		{
			Array<Named *> *readFns = objStream.type->findName(new (this) Str(S("readValue")));
			for (Nat i = 0; i < readFns->count(); i++) {
				Named *fn = readFns->at(i);
				if (fn->params->count() == 3
					&& fn->params->at(0).type == objStream.type
					&& fn->params->at(1).type == StormInfo<Type>::type(engine)) {
					readValueFn = as<Function>(fn);
					break;
				}
			}
		}
		Function *natReadFn = findStormFn(natType, S("read"), objStream);

		Listing *l = new (this) Listing(false, natType.desc(engine));
		code::Var streamVar = l->createParam(objStream.desc(engine));
		code::Var resultVar = l->createVar(l->root(), natType.desc(engine));

		*l << prolog();

		// Call "readValue".
		*l << lea(ptrA, resultVar);
		*l << fnParam(objStream.desc(engine), streamVar);
		*l << fnParam(engine.ptrDesc(), this->typeRef());
		*l << fnParam(engine.ptrDesc(), ptrA);
		*l << fnCall(readValueFn->ref(), true);

		// Done!
		*l << fnRet(resultVar);

		Array<Value> *params = new (this) Array<Value>();
		params->reserve(1);
		*params << objStream;
		Function *fn = new (this) Function(Value(this), new (this) Str(S("read")), params);
		fn->setCode(new (this) DynamicCode(l));
		fn->make(fnStatic);
		return fn;
	}

	Function *Enum::createReadCtor() {
		using namespace code;

		Value me = thisPtr(this);
		Value objStream(StormInfo<ObjIStream>::type(engine));
		Value natType(StormInfo<Nat>::type(engine));

		Function *natReadFn = findStormFn(natType, S("read"), objStream);
		Function *endFn = findStormMemberFn(objStream, S("end"));

		Listing *l = new (this) Listing(true, engine.voidDesc());
		code::Var meVar = l->createParam(me.desc(engine));
		code::Var streamVar = l->createParam(objStream.desc(engine));

		*l << prolog();

		// Call "read" for a Nat.
		*l << fnParam(objStream.desc(engine), streamVar);
		*l << fnCall(natReadFn->ref(), false, natType.desc(engine), eax);

		// Initialize the enum by writing the value to it.
		*l << mov(ptrC, meVar);
		*l << mov(intRel(ptrC, Offset()), eax);

		// Call "end".
		*l << fnParam(objStream.desc(engine), streamVar);
		*l << fnCall(endFn->ref(), true);

		// Then we are done.
		*l << fnRet();

		Array<Value> *params = new (this) Array<Value>();
		params->reserve(2);
		*params << me << objStream;
		Function *fn = new (this) Function(Value(), new (this) Str(Type::CTOR), params);
		fn->setCode(new (this) DynamicCode(l));
		fn->visibility = typePrivate(engine);
		return fn;
	}

	EnumValue::EnumValue(Enum *owner, Str *name, Nat value)
		: Function(Value(owner), name, new (name) Array<Value>()),
		  value(value) {

		setCode(new (this) InlineCode(fnPtr(engine(), &EnumValue::generate, this)));
	}

	void EnumValue::toS(StrBuf *to) const {
		*to << identifier() << S(" = ") << value;
	}

	void EnumValue::generate(InlineParams p) {
		if (!p.result->needed())
			return;

		Operand result = p.result->location(p.state);
		*p.state->l << mov(result, natConst(value));
	}

	EnumOutput::EnumOutput() : Template(new (engine()) Str(S("<<"))) {}

	MAYBE(Named *) EnumOutput::generate(SimplePart *part) {
		Array<Value> *params = part->params;
		if (params->count() != 2)
			return null;

		Value strBuf = Value(StrBuf::stormType(engine()));
		if (params->at(0).type != strBuf.type)
			return null;

		Value e = params->at(1).asRef(false);
		Enum *type = as<Enum>(e.type);
		if (!type)
			return null;

		TypeDesc *ptr = engine().ptrDesc();
		Listing *l = new (this) Listing(true, ptr);
		Var out = l->createParam(ptr);
		Var in = l->createParam(intDesc(engine()));

		*l << prolog();

		*l << fnParam(ptr, type->typeRef());
		*l << fnParam(ptr, out);
		*l << fnParam(intDesc(engine()), in);
		*l << fnCall(engine().ref(builtin::enumToS), true);

		*l << fnRet(out);

		return dynamicFunction(engine(), strBuf, S("<<"), valList(engine(), 2, strBuf, e), l);
	}

}
