#include "stdafx.h"
#include "Enum.h"
#include "Number.h"
#include "Engine.h"
#include "Core/Hash.h"
#include "Function.h"

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
			// TODO: Use 'test' op-code when it is implemented.
			Var t = p.state->l->createVar(p.state->block, Size::sInt);
			*p.state->l << mov(t, p.param(0));
			*p.state->l << band(t, p.param(1));
			*p.state->l << cmp(t, intConst(0));
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
