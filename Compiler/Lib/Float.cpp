#include "stdafx.h"
#include "Float.h"
#include "Function.h"
#include "Number.h"
#include "Core/Io/Serialization.h"
#include "Core/Io/SerializationUtils.h"

namespace storm {
	using namespace code;

	Type *createFloat(Str *name, Size size, GcType *type) {
		return new (name) FloatType(name, type);
	}

	static void floatCopyCtor(InlineParams p) {
		p.allocRegs(0, 1);
		*p.state->l << mov(floatRel(p.regParam(0), Offset()), floatRel(p.regParam(1), Offset()));
	}

	static void floatAdd(InlineParams p) {
		if (p.result->needed()) {
			Operand result = p.result->location(p.state);
			*p.state->l << mov(result, p.param(0));
			*p.state->l << fadd(result, p.param(1));
		}
	}

	static void floatSub(InlineParams p) {
		if (p.result->needed()) {
			Operand result = p.result->location(p.state);
			*p.state->l << mov(result, p.param(0));
			*p.state->l << fsub(result, p.param(1));
		}
	}

	static void floatNeg(InlineParams p) {
		if (p.result->needed()) {
			Operand result = p.result->location(p.state);
			*p.state->l << fneg(result, p.param(0));
		}
	}

	static void floatMul(InlineParams p) {
		if (p.result->needed()) {
			Operand result = p.result->location(p.state);
			*p.state->l << mov(result, p.param(0));
			*p.state->l << fmul(result, p.param(1));
		}
	}

	static void floatDiv(InlineParams p) {
		if (p.result->needed()) {
			Operand result = p.result->location(p.state);
			*p.state->l << mov(result, p.param(0));
			*p.state->l << fdiv(result, p.param(1));
		}
	}

	static void floatAssign(InlineParams p) {
		p.allocRegs(0);
		Reg dest = p.regParam(0);

		*p.state->l << mov(floatRel(dest, Offset()), p.param(1));
		if (p.result->needed())
			if (!p.result->suggest(p.state, p.originalParam(0)))
				*p.state->l << mov(p.result->location(p.state), dest);
	}

	// Note: Can be used for 'double' or 'float' -> 'int' or 'long'.
	static void floatToInt(InlineParams p) {
		if (!p.result->needed())
			return;
		*p.state->l << fcasti(p.result->location(p.state), p.param(0));
	}

	static void floatToNat(InlineParams p) {
		if (!p.result->needed())
			return;
		*p.state->l << fcastu(p.result->location(p.state), p.param(0));
	}

	// Note: Can be used for both 'double' and 'float'.
	static void floatToFloat(InlineParams p) {
		if (!p.result->needed())
			return;
		*p.state->l << fcast(p.result->location(p.state), p.param(0));
	}

	template <CondFlag f>
	static void floatCmp(InlineParams p) {
		if (p.result->needed()) {
			Operand result = p.result->location(p.state);
			*p.state->l << fcmp(p.param(0), p.param(1));
			*p.state->l << setCond(result, f);
		}
	}

	static Float floatMin(Float a, Float b) {
		return min(a, b);
	}

	static Float floatMax(Float a, Float b) {
		return max(a, b);
	}

	static Float floatRead(IStream *from) {
		return from->readFloat();
	}

	static Float floatReadS(ObjIStream *from) {
		return Serialize<Float>::read(from);
	}

	static void floatWrite(Float v, OStream *to) {
		to->writeFloat(v);
	}

	static void floatWriteS(Float v, ObjOStream *to) {
		Serialize<Float>::write(v, to);
	}

	FloatType::FloatType(Str *name, GcType *type) : Type(name, typeValue | typeFinal, Size::sFloat, type, null) {}

	Bool FloatType::loadAll() {
		//Array<Value> *r = new (this) Array<Value>(1, Value(this, true));
		Array<Value> *v = new (this) Array<Value>(1, Value(this, false));
		Array<Value> *rr = new (this) Array<Value>(2, Value(this, true));
		Array<Value> *vv = new (this) Array<Value>(2, Value(this, false));
		Array<Value> *rv = new (this) Array<Value>(2, Value(this, true));
		rv->at(1) = Value(this);

		add(inlinedFunction(engine, Value(), Type::CTOR, rr, fnPtr(engine, &floatCopyCtor))->makePure());
		add(inlinedFunction(engine, Value(this, true), S("="), rv, fnPtr(engine, &floatAssign))->makePure());

		add(inlinedFunction(engine, Value(this), S("+"), vv, fnPtr(engine, &floatAdd))->makePure());
		add(inlinedFunction(engine, Value(this), S("-"), vv, fnPtr(engine, &floatSub))->makePure());
		add(inlinedFunction(engine, Value(this), S("*"), vv, fnPtr(engine, &floatMul))->makePure());
		add(inlinedFunction(engine, Value(this), S("/"), vv, fnPtr(engine, &floatDiv))->makePure());
		add(inlinedFunction(engine, Value(this), S("-"), v, fnPtr(engine, &floatNeg))->makePure());

		Value vBool = Value(StormInfo<Bool>::type(engine));
		add(inlinedFunction(engine, vBool, S(">"), vv, fnPtr(engine, &floatCmp<ifFAbove>))->makePure());
		add(inlinedFunction(engine, vBool, S(">="), vv, fnPtr(engine, &floatCmp<ifFAboveEqual>))->makePure());
		add(inlinedFunction(engine, vBool, S("<"), vv, fnPtr(engine, &floatCmp<ifFBelow>))->makePure());
		add(inlinedFunction(engine, vBool, S("<="), vv, fnPtr(engine, &floatCmp<ifFBelowEqual>))->makePure());
		add(inlinedFunction(engine, vBool, S("=="), vv, fnPtr(engine, &floatCmp<ifEqual>))->makePure());
		add(inlinedFunction(engine, vBool, S("!="), vv, fnPtr(engine, &floatCmp<ifNotEqual>))->makePure());

		Value vInt = Value(StormInfo<Int>::type(engine));
		add(inlinedFunction(engine, vInt, S("int"), v, fnPtr(engine, &floatToInt))->makePure());
		Value vLong = Value(StormInfo<Long>::type(engine));
		add(inlinedFunction(engine, vLong, S("long"), v, fnPtr(engine, &floatToInt))->makePure());
		TODO(L"Add conversions to nat and word as well.");
		Value vDouble = Value(StormInfo<Double>::type(engine));
		add(inlinedFunction(engine, vDouble, S("double"), v, fnPtr(engine, &floatToFloat))->makePure());

		add(nativeFunction(engine, Value(this), S("min"), vv, address(&floatMin))->makePure());
		add(nativeFunction(engine, Value(this), S("max"), vv, address(&floatMax))->makePure());

		Array<Value> *is = new (this) Array<Value>(1, Value(StormInfo<IStream>::type(engine)));
		add(nativeFunction(engine, Value(this), S("read"), is, address(&floatRead)));

		is = new (this) Array<Value>(1, Value(StormInfo<ObjIStream>::type(engine)));
		add(nativeFunction(engine, Value(this), S("read"), is, address(&floatReadS)));

		Array<Value> *os = new (this) Array<Value>(2, Value(this, false));
		os->at(1) = Value(StormInfo<OStream>::type(engine));
		add(nativeFunction(engine, Value(), S("write"), os, address(&floatWrite)));

		os = new (this) Array<Value>(2, Value(this, false));
		os->at(1) = Value(StormInfo<ObjOStream>::type(engine));
		add(nativeFunction(engine, Value(), S("write"), os, address(&floatWriteS)));

		return Type::loadAll();
	}


	Type *createDouble(Str *name, Size size, GcType *type) {
		return new (name) DoubleType(name, type);
	}

	static void doubleCopyCtor(InlineParams p) {
		p.allocRegs(0, 1);
		*p.state->l << mov(doubleRel(p.regParam(0), Offset()), doubleRel(p.regParam(1), Offset()));
	}

	static void doubleAssign(InlineParams p) {
		p.allocRegs(0);
		Reg dest = p.regParam(0);

		*p.state->l << mov(doubleRel(dest, Offset()), p.param(1));
		if (p.result->needed())
			if (!p.result->suggest(p.state, p.originalParam(0))) {
				*p.state->l << mov(p.result->location(p.state), dest);
			}
	}

	static Double doubleMin(Double a, Double b) {
		return min(a, b);
	}

	static Double doubleMax(Double a, Double b) {
		return max(a, b);
	}

	static void castDouble(InlineParams p) {
		p.allocRegs(0);
		*p.state->l << fld(p.param(1));
		*p.state->l << fstp(doubleRel(p.regParam(0), Offset()));
	}

	static Double doubleRead(IStream *from) {
		return from->readDouble();
	}

	static Double doubleReadS(ObjIStream *from) {
		return Serialize<Double>::read(from);
	}

	static void doubleWrite(Double v, OStream *to) {
		to->writeDouble(v);
	}

	static void doubleWriteS(Double v, ObjOStream *to) {
		Serialize<Double>::write(v, to);
	}

	DoubleType::DoubleType(Str *name, GcType *type) : Type(name, typeValue | typeFinal, Size::sDouble, type, null) {}

	Bool DoubleType::loadAll() {
		//Array<Value> *r = new (this) Array<Value>(1, Value(this, true));
		Array<Value> *v = new (this) Array<Value>(1, Value(this, false));
		Array<Value> *rr = new (this) Array<Value>(2, Value(this, true));
		Array<Value> *vv = new (this) Array<Value>(2, Value(this, false));
		Array<Value> *rv = new (this) Array<Value>(2, Value(this, true));
		rv->at(1) = Value(this);

		add(inlinedFunction(engine, Value(), Type::CTOR, rr, fnPtr(engine, &doubleCopyCtor))->makePure());
		add(inlinedFunction(engine, Value(this, true), S("="), rv, fnPtr(engine, &doubleAssign))->makePure());

		add(inlinedFunction(engine, Value(this), S("+"), vv, fnPtr(engine, &floatAdd))->makePure());
		add(inlinedFunction(engine, Value(this), S("-"), vv, fnPtr(engine, &floatSub))->makePure());
		add(inlinedFunction(engine, Value(this), S("*"), vv, fnPtr(engine, &floatMul))->makePure());
		add(inlinedFunction(engine, Value(this), S("/"), vv, fnPtr(engine, &floatDiv))->makePure());
		add(inlinedFunction(engine, Value(this), S("-"), v, fnPtr(engine, &floatNeg))->makePure());

		Value vBool = Value(StormInfo<Bool>::type(engine));
		add(inlinedFunction(engine, vBool, S(">"), vv, fnPtr(engine, &floatCmp<ifFAbove>))->makePure());
		add(inlinedFunction(engine, vBool, S(">="), vv, fnPtr(engine, &floatCmp<ifFAboveEqual>))->makePure());
		add(inlinedFunction(engine, vBool, S("<"), vv, fnPtr(engine, &floatCmp<ifFBelow>))->makePure());
		add(inlinedFunction(engine, vBool, S("<="), vv, fnPtr(engine, &floatCmp<ifFBelowEqual>))->makePure());
		add(inlinedFunction(engine, vBool, S("=="), vv, fnPtr(engine, &floatCmp<ifEqual>))->makePure());
		add(inlinedFunction(engine, vBool, S("!="), vv, fnPtr(engine, &floatCmp<ifNotEqual>))->makePure());

		Array<Value> *rf = valList(engine, 2, Value(this, true), Value(StormInfo<Float>::type(engine)));
		add(inlinedFunction(engine, Value(), Type::CTOR, rf, fnPtr(engine, &castDouble))->makeAutoCast()->makePure());

		Value vInt = Value(StormInfo<Int>::type(engine));
		add(inlinedFunction(engine, vInt, S("int"), v, fnPtr(engine, &floatToInt))->makePure());
		Value vLong = Value(StormInfo<Long>::type(engine));
		add(inlinedFunction(engine, vLong, S("long"), v, fnPtr(engine, &floatToInt))->makePure()); // We can use the same function here!
		Value vFloat = Value(StormInfo<Float>::type(engine));
		add(inlinedFunction(engine, vFloat, S("float"), v, fnPtr(engine, &floatToFloat))->makePure());

		add(nativeFunction(engine, Value(this), S("min"), vv, address(&doubleMin))->makePure());
		add(nativeFunction(engine, Value(this), S("max"), vv, address(&doubleMax))->makePure());

		Array<Value> *is = new (this) Array<Value>(1, Value(StormInfo<IStream>::type(engine)));
		add(nativeFunction(engine, Value(this), S("read"), is, address(&doubleRead)));

		Array<Value> *os = new (this) Array<Value>(2, Value(this, false));
		os->at(1) = Value(StormInfo<OStream>::type(engine));
		add(nativeFunction(engine, Value(), S("write"), os, address(&doubleWrite)));

		os = new (this) Array<Value>(2, Value(this, false));
		os->at(1) = Value(StormInfo<ObjOStream>::type(engine));
		add(nativeFunction(engine, Value(), S("write"), os, address(&doubleWriteS)));

		return Type::loadAll();
	}

}
