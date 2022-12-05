#include "stdafx.h"
#include "Int.h"
#include "Core/Array.h"
#include "Core/Hash.h"
#include "Core/Str.h"
#include "Core/Io/Serialization.h"
#include "Core/Io/SerializationUtils.h"
#include "Function.h"
#include "Number.h"

namespace storm {
	using namespace code;

	Type *createInt(Str *name, Size size, GcType *type) {
		return new (name) IntType(name, type);
	}

	// Note: Works for both float and double.
	static void intToFloat(InlineParams p) {
		if (!p.result->needed())
			return;

		*p.state->l << icastf(p.result->location(p.state), p.param(0));
	}

	static Int intRead(IStream *from) {
		return from->readInt();
	}

	static Int intReadS(ObjIStream *from) {
		return Serialize<Int>::read(from);
	}

	static void intWrite(Int v, OStream *to) {
		to->writeInt(v);
	}

	static void intWriteS(Int v, ObjOStream *to) {
		Serialize<Int>::write(v, to);
	}

	IntType::IntType(Str *name, GcType *type) : Type(name, typeValue | typeFinal, Size::sInt, type, null) {}

	Bool IntType::loadAll() {
		Array<Value> *r = new (this) Array<Value>(1, Value(this, true));
		Array<Value> *rr = new (this) Array<Value>(2, Value(this, true));
		Array<Value> *v = new (this) Array<Value>(1, Value(this, false));
		Array<Value> *vv = new (this) Array<Value>(2, Value(this, false));
		Array<Value> *rv = new (this) Array<Value>(2, Value(this, true));
		rv->at(1) = Value(this);
		Value b(StormInfo<Bool>::type(engine));

		add(inlinedFunction(engine, Value(this), S("+"), vv, fnPtr(engine, &numAdd))->makePure());
		add(inlinedFunction(engine, Value(this), S("-"), vv, fnPtr(engine, &numSub))->makePure());
		add(inlinedFunction(engine, Value(this), S("*"), vv, fnPtr(engine, &numMul))->makePure());
		add(inlinedFunction(engine, Value(this), S("/"), vv, fnPtr(engine, &numIDiv))->makePure());
		add(inlinedFunction(engine, Value(this), S("%"), vv, fnPtr(engine, &numIMod))->makePure());
		add(inlinedFunction(engine, Value(this), S("-"), v, fnPtr(engine, &numINeg))->makePure());

		add(inlinedFunction(engine, b, S("=="), vv, fnPtr(engine, &numCmp<ifEqual>))->makePure());
		add(inlinedFunction(engine, b, S("!="), vv, fnPtr(engine, &numCmp<ifNotEqual>))->makePure());
		add(inlinedFunction(engine, b, S("<="), vv, fnPtr(engine, &numCmp<ifLessEqual>))->makePure());
		add(inlinedFunction(engine, b, S(">="), vv, fnPtr(engine, &numCmp<ifGreaterEqual>))->makePure());
		add(inlinedFunction(engine, b, S("<"), vv, fnPtr(engine, &numCmp<ifLess>))->makePure());
		add(inlinedFunction(engine, b, S(">"), vv, fnPtr(engine, &numCmp<ifGreater>))->makePure());

		add(inlinedFunction(engine, Value(this), S("*++"), r, fnPtr(engine, &numPostfixInc<Int>))->makePure());
		add(inlinedFunction(engine, Value(this), S("++*"), r, fnPtr(engine, &numPrefixInc<Int>))->makePure());
		add(inlinedFunction(engine, Value(this), S("*--"), r, fnPtr(engine, &numPostfixDec<Int>))->makePure());
		add(inlinedFunction(engine, Value(this), S("--*"), r, fnPtr(engine, &numPrefixDec<Int>))->makePure());

		add(inlinedFunction(engine, Value(this, true), S("="), rv, fnPtr(engine, &numAssign<Int>))->makePure());
		add(inlinedFunction(engine, Value(this), S("+="), rv, fnPtr(engine, &numInc<Int>))->makePure());
		add(inlinedFunction(engine, Value(this), S("-="), rv, fnPtr(engine, &numDec<Int>))->makePure());
		add(inlinedFunction(engine, Value(this), S("*="), rv, fnPtr(engine, &numScale<Int>))->makePure());
		add(inlinedFunction(engine, Value(this), S("/="), rv, fnPtr(engine, &numIDivScale<Int>))->makePure());
		add(inlinedFunction(engine, Value(this), S("%="), rv, fnPtr(engine, &numIModEq<Int>))->makePure());

		add(inlinedFunction(engine, Value(), Type::CTOR, rr, fnPtr(engine, &numCopyCtor<Int>))->makePure());
		add(inlinedFunction(engine, Value(), Type::CTOR, r, fnPtr(engine, &numInit<Int>))->makePure());

		add(inlinedFunction(engine, Value(StormInfo<Nat>::type(engine)), S("nat"), v, fnPtr(engine, &icast))->makePure());
		add(inlinedFunction(engine, Value(StormInfo<Byte>::type(engine)), S("byte"), v, fnPtr(engine, &icast))->makePure());
		add(inlinedFunction(engine, Value(StormInfo<Long>::type(engine)), S("long"), v, fnPtr(engine, &icast))->makePure());
		add(inlinedFunction(engine, Value(StormInfo<Word>::type(engine)), S("word"), v, fnPtr(engine, &icast))->makePure());
		add(inlinedFunction(engine, Value(StormInfo<Float>::type(engine)), S("float"), v, fnPtr(engine, &intToFloat))->makePure());
		add(inlinedFunction(engine, Value(StormInfo<Double>::type(engine)), S("double"), v, fnPtr(engine, &intToFloat))->makePure());

		Value n(StormInfo<Nat>::type(engine));
		add(nativeFunction(engine, n, S("hash"), v, address(&intHash))->makePure());
		add(inlinedFunction(engine, Value(this), S("min"), vv, fnPtr(engine, &numMin<Int>))->makePure());
		add(inlinedFunction(engine, Value(this), S("max"), vv, fnPtr(engine, &numMax<Int>))->makePure());
		add(inlinedFunction(engine, Value(this), S("delta"), vv, fnPtr(engine, &numDelta<Int>))->makePure());

		Array<Value> *is = new (this) Array<Value>(1, Value(StormInfo<IStream>::type(engine)));
		add(nativeFunction(engine, Value(this), S("read"), is, address(&intRead)));

		is = new (this) Array<Value>(1, Value(StormInfo<ObjIStream>::type(engine)));
		add(nativeFunction(engine, Value(this), S("read"), is, address(&intReadS)));

		Array<Value> *os = new (this) Array<Value>(2, Value(this, false));
		os->at(1) = Value(StormInfo<OStream>::type(engine));
		add(nativeFunction(engine, Value(), S("write"), os, address(&intWrite)));

		os = new (this) Array<Value>(2, Value(this, false));
		os->at(1) = Value(StormInfo<ObjOStream>::type(engine));
		add(nativeFunction(engine, Value(), S("write"), os, address(&intWriteS)));

		return Type::loadAll();
	}


	Type *createNat(Str *name, Size size, GcType *type) {
		return new (name) NatType(name, type);
	}

	static void castNat(InlineParams p) {
		p.allocRegs(0);
		*p.state->l << ucast(intRel(p.regParam(0), Offset()), p.param(1));
	}

	static void natToFloat(InlineParams p) {
		if (!p.result->needed())
			return;

		*p.state->l << ucastf(p.result->location(p.state), p.param(0));
	}

	static Nat natRead(IStream *from) {
		return from->readNat();
	}

	static Nat natReadS(ObjIStream *from) {
		return Serialize<Nat>::read(from);
	}

	static void natWrite(Nat v, OStream *to) {
		to->writeNat(v);
	}

	static void natWriteS(Nat v, ObjOStream *to) {
		Serialize<Nat>::write(v, to);
	}


	NatType::NatType(Str *name, GcType *type) : Type(name, typeValue | typeFinal, Size::sNat, type, null) {}

	Bool NatType::loadAll() {
		Array<Value> *r = new (this) Array<Value>(1, Value(this, true));
		Array<Value> *rr = new (this) Array<Value>(2, Value(this, true));
		Array<Value> *v = new (this) Array<Value>(1, Value(this, false));
		Array<Value> *vv = new (this) Array<Value>(2, Value(this, false));
		Array<Value> *rv = new (this) Array<Value>(2, Value(this, true));
		rv->at(1) = Value(this);
		Value b(StormInfo<Bool>::type(engine));

		Array<Value> *vb = new (this) Array<Value>(2, Value(this, false));
		Array<Value> *rb = new (this) Array<Value>(2, Value(this, true));
		vb->at(1) = rb->at(1) = Value(StormInfo<Byte>::type(engine));

		add(inlinedFunction(engine, Value(this), S("+"), vv, fnPtr(engine, &numAdd))->makePure());
		add(inlinedFunction(engine, Value(this), S("-"), vv, fnPtr(engine, &numSub))->makePure());
		add(inlinedFunction(engine, Value(this), S("*"), vv, fnPtr(engine, &numMul))->makePure());
		add(inlinedFunction(engine, Value(this), S("/"), vv, fnPtr(engine, &numUDiv))->makePure());
		add(inlinedFunction(engine, Value(this), S("%"), vv, fnPtr(engine, &numUMod))->makePure());

		add(inlinedFunction(engine, b, S("=="), vv, fnPtr(engine, &numCmp<ifEqual>))->makePure());
		add(inlinedFunction(engine, b, S("!="), vv, fnPtr(engine, &numCmp<ifNotEqual>))->makePure());
		add(inlinedFunction(engine, b, S("<="), vv, fnPtr(engine, &numCmp<ifBelowEqual>))->makePure());
		add(inlinedFunction(engine, b, S(">="), vv, fnPtr(engine, &numCmp<ifAboveEqual>))->makePure());
		add(inlinedFunction(engine, b, S("<"), vv, fnPtr(engine, &numCmp<ifBelow>))->makePure());
		add(inlinedFunction(engine, b, S(">"), vv, fnPtr(engine, &numCmp<ifAbove>))->makePure());

		add(inlinedFunction(engine, Value(this), S("*++"), r, fnPtr(engine, &numPostfixInc<Nat>))->makePure());
		add(inlinedFunction(engine, Value(this), S("++*"), r, fnPtr(engine, &numPrefixInc<Nat>))->makePure());
		add(inlinedFunction(engine, Value(this), S("*--"), r, fnPtr(engine, &numPostfixDec<Nat>))->makePure());
		add(inlinedFunction(engine, Value(this), S("--*"), r, fnPtr(engine, &numPrefixDec<Nat>))->makePure());

		add(inlinedFunction(engine, Value(this, true), S("="), rv, fnPtr(engine, &numAssign<Nat>))->makePure());
		add(inlinedFunction(engine, Value(this), S("+="), rv, fnPtr(engine, &numInc<Nat>))->makePure());
		add(inlinedFunction(engine, Value(this), S("-="), rv, fnPtr(engine, &numDec<Nat>))->makePure());
		add(inlinedFunction(engine, Value(this), S("*="), rv, fnPtr(engine, &numScale<Nat>))->makePure());
		add(inlinedFunction(engine, Value(this), S("/="), rv, fnPtr(engine, &numUDivScale<Nat>))->makePure());
		add(inlinedFunction(engine, Value(this), S("%="), rv, fnPtr(engine, &numUModEq<Nat>))->makePure());

		// Bitwise operators.
		add(inlinedFunction(engine, Value(this), S("&"), vv, fnPtr(engine, &numAnd))->makePure());
		add(inlinedFunction(engine, Value(this), S("|"), vv, fnPtr(engine, &numOr))->makePure());
		add(inlinedFunction(engine, Value(this), S("^"), vv, fnPtr(engine, &numXor))->makePure());
		add(inlinedFunction(engine, Value(this), S("~"), v, fnPtr(engine, &numNot))->makePure());
		add(inlinedFunction(engine, Value(this), S("<<"), vb, fnPtr(engine, &numShl))->makePure());
		add(inlinedFunction(engine, Value(this), S(">>"), vb, fnPtr(engine, &numShr))->makePure());

		add(inlinedFunction(engine, Value(this), S("&="), rv, fnPtr(engine, &numAndEq<Nat>))->makePure());
		add(inlinedFunction(engine, Value(this), S("|="), rv, fnPtr(engine, &numOrEq<Nat>))->makePure());
		add(inlinedFunction(engine, Value(this), S("^="), rv, fnPtr(engine, &numXorEq<Nat>))->makePure());
		add(inlinedFunction(engine, Value(this), S("<<="), rb, fnPtr(engine, &numShlEq<Nat>))->makePure());
		add(inlinedFunction(engine, Value(this), S(">>="), rb, fnPtr(engine, &numShrEq<Nat>))->makePure());


		add(inlinedFunction(engine, Value(), Type::CTOR, rr, fnPtr(engine, &numCopyCtor<Nat>))->makePure());
		add(inlinedFunction(engine, Value(), Type::CTOR, r, fnPtr(engine, &numInit<Nat>))->makePure());

		add(inlinedFunction(engine, Value(), Type::CTOR, rb, fnPtr(engine, &castNat))->makeAutoCast()->makePure());

		add(inlinedFunction(engine, Value(StormInfo<Int>::type(engine)), S("int"), v, fnPtr(engine, &ucast))->makePure());
		add(inlinedFunction(engine, Value(StormInfo<Byte>::type(engine)), S("byte"), v, fnPtr(engine, &ucast))->makePure());
		add(inlinedFunction(engine, Value(StormInfo<Long>::type(engine)), S("long"), v, fnPtr(engine, &ucast))->makePure());
		add(inlinedFunction(engine, Value(StormInfo<Word>::type(engine)), S("word"), v, fnPtr(engine, &ucast))->makePure());
		add(inlinedFunction(engine, Value(StormInfo<Float>::type(engine)), S("float"), v, fnPtr(engine, &natToFloat))->makePure());
		add(inlinedFunction(engine, Value(StormInfo<Double>::type(engine)), S("double"), v, fnPtr(engine, &natToFloat))->makePure());

		add(nativeFunction(engine, Value(this), S("hash"), v, address(&natHash))->makePure());
		add(inlinedFunction(engine, Value(this), S("min"), vv, fnPtr(engine, &numMin<Nat>))->makePure());
		add(inlinedFunction(engine, Value(this), S("max"), vv, fnPtr(engine, &numMax<Nat>))->makePure());
		add(inlinedFunction(engine, Value(this), S("delta"), vv, fnPtr(engine, &numDelta<Nat>))->makePure());

		Array<Value> *is = new (this) Array<Value>(1, Value(StormInfo<IStream>::type(engine)));
		add(nativeFunction(engine, Value(this), S("read"), is, address(&natRead)));

		is = new (this) Array<Value>(1, Value(StormInfo<ObjIStream>::type(engine)));
		add(nativeFunction(engine, Value(this), S("read"), is, address(&natReadS)));

		Array<Value> *os = new (this) Array<Value>(2, Value(this, false));
		os->at(1) = Value(StormInfo<OStream>::type(engine));
		add(nativeFunction(engine, Value(), S("write"), os, address(&natWrite)));

		os = new (this) Array<Value>(2, Value(this, false));
		os->at(1) = Value(StormInfo<ObjOStream>::type(engine));
		add(nativeFunction(engine, Value(), S("write"), os, address(&natWriteS)));

		return Type::loadAll();
	}

}
