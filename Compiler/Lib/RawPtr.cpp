#include "stdafx.h"
#include "RawPtr.h"
#include "Compiler/Engine.h"
#include "Compiler/Variable.h"
#include "Number.h"
#include "Maybe.h"
#include "Core/Hash.h"
#include "Core/Variant.h"

namespace storm {

	void rawPtrCopy(InlineParams p) {
		p.allocRegs(0, 1);
		*p.state->l << mov(ptrRel(p.regParam(0)), ptrRel(p.regParam(1)));
	}

	void rawPtrInit(InlineParams p) {
		p.allocRegs(0);
		*p.state->l << mov(ptrRel(p.regParam(0)), p.param(1));
	}

	void rawPtrInitEmpty(InlineParams p) {
		p.allocRegs(0);
		*p.state->l << mov(ptrRel(p.regParam(0)), ptrConst(Size()));
	}

	void rawPtrAssign(InlineParams p) {
		using namespace code;

		p.allocRegs(0);
		code::Reg dest = p.regParam(0);

		*p.state->l << mov(ptrRel(dest, Offset()), p.param(1));
		if (p.result->needed()) {
			if (p.result->type().ref) {
				// Suggest params[0].
				if (!p.result->suggest(p.state, p.originalParam(0)))
					*p.state->l << mov(p.result->location(p.state), dest);
			} else {
				// Suggest params[1].
				if (!p.result->suggest(p.state, p.param(1)))
					*p.state->l << mov(p.result->location(p.state), dest);
			}
		}
	}

	void rawPtrEmpty(InlineParams p) {
		using namespace code;

		if (p.result->needed()) {
			*p.state->l << cmp(p.param(0), ptrConst(0));
			*p.state->l << setCond(p.result->location(p.state), ifEqual);
		}
	}

	void rawPtrAny(InlineParams p) {
		using namespace code;

		if (p.result->needed()) {
			*p.state->l << cmp(p.param(0), ptrConst(0));
			*p.state->l << setCond(p.result->location(p.state), ifNotEqual);
		}
	}

	void rawPtrGet(InlineParams p) {
		using namespace code;

		if (p.result->needed()) {
			if (p.result->type().ref) {
				*p.state->l << lea(p.result->location(p.state), p.param(0));
			} else if (!p.result->suggest(p.state, p.param(0))) {
				*p.state->l << mov(p.result->location(p.state), p.param(0));
			}
		}
	}

	Str *CODECALL rawPtrToS(EnginePtr e, RootObject *ptr) {
		StrBuf *buf = new (e) StrBuf();
		*buf << S("0x") << hex(ptr);
		return buf->toS();
	}

	Type *CODECALL rawPtrType(RootObject *ptr) {
		if (ptr)
			return runtime::typeOf(ptr);
		else
			return null;
	}

	Bool CODECALL rawPtrIsValue(const void *ptr) {
		if (!ptr)
			return false;
		const GcType *type = runtime::gcTypeOf(ptr);
		if (type->kind == GcType::tArray || type->kind == GcType::tWeakArray)
			return type->type != null;
		else
			return false;
	}

	template <class T>
	T CODECALL rawPtrRead(const void *ptr, Nat offset) {
		const GcType *t = runtime::gcTypeOf(ptr);
		if (t->kind == GcType::tArray || t->kind == GcType::tWeakArray) {
			offset += OFFSET_OF(GcArray<Byte>, v);
		}

		return *(T *)((byte *)ptr + offset);
	}

	Nat CODECALL rawPtrCount(const void *ptr) {
		if (!ptr)
			return 0;

		const GcType *t = runtime::gcTypeOf(ptr);
		if (t->kind == GcType::tArray) {
			return Nat(((GcArray<byte> *)ptr)->count);
		} else if (t->kind == GcType::tWeakArray) {
			return Nat(((GcWeakArray<void *> *)ptr)->count());
		} else {
			return 1;
		}
	}

	Nat CODECALL rawPtrFilled(const void *ptr) {
		if (!ptr)
			return 0;

		const GcType *t = runtime::gcTypeOf(ptr);
		if (t->kind == GcType::tArray) {
			return Nat(((GcArray<byte> *)ptr)->filled);
		} else if (t->kind == GcType::tWeakArray) {
			return Nat(((GcWeakArray<void *> *)ptr)->splatted());
		} else {
			return 1;
		}
	}

	Nat CODECALL rawPtrSize(const void *ptr) {
		if (!ptr)
			return 0;

		const GcType *t = runtime::gcTypeOf(ptr);
		return Nat(t->stride);
	}

	// To make the calling convention behave correctly.
	struct DummyPtr : GcArray<byte> {
		Variant CODECALL rawPtrVariant() {
			// Note: this is to make sure the compiler does not remove the null check on "this". It
			// is technically UB to have a null this-pointer, but in these circumstances it can
			// occur so we need to check it here.
			// We use atomic reads/writes to ensure that the compiler does not optimize the check
			// away. We also have a test for this so that we get to know if this breaks in the future.
			void *obj = this;
			obj = atomicRead(obj);
			if (!obj)
				return Variant();

			const GcType *t = runtime::gcTypeOf(obj);
			if (t->kind == GcType::tArray) {
				// We can't express arrays as variants.
				if (count != 1 || filled != 1)
					return Variant();

				if (!t->type)
					return Variant();

				// Create a variant!
				return Variant(v, t->type);
			} else if (t->kind == GcType::tWeakArray) {
				// We don't support weak arrays.
				return Variant();
			} else if (t->kind == GcType::tFixed || t->kind == GcType::tFixedObj || t->kind == GcType::tType) {
				// A pointer to some object.
				return Variant((RootObject *)obj);
			} else {
				return Variant();
			}
		}
	};

	Bool CODECALL rawPtrCopyVariant(void *to, Variant &variant) {
		if (!to)
			return false;

		const GcType *t = runtime::gcTypeOf(to);
		if (t->kind == GcType::tArray) {
			GcArray<byte> *array = (GcArray<byte> *)to;
			// We can't express arrays as variants.
			if (array->count != 1 || !t->type)
				return false;

			if (!variant.has(t->type))
				return false;

			const Handle &handle = t->type->handle();

			// If 'filled', then call the destructor.
			if (array->filled)
				handle.safeDestroy(array->v);

			handle.safeCopy(array->v, variant.getValue());
			array->filled = 1;
			return true;
		} else if (t->kind == GcType::tWeakArray) {
			// We don't support weak arrays.
			return false;
		} else if (t->kind == GcType::tFixed || t->kind == GcType::tFixedObj || t->kind == GcType::tType) {
			// A pointer to some object.
			if (!t->type)
				return false;
			if (!variant.has(t->type))
				return false;

			Type::CopyCtorFn copyCtor = t->type->rawCopyConstructor();
			if (!copyCtor)
				return false;
			(*copyCtor)(to, variant.getObject());
			return true;
		} else {
			return false;
		}
	}

	void *CODECALL rawPtrAllocArray(Type *type, Nat count) {
		const GcType *t = type->handle().gcArrayType;
		return runtime::allocArray(type->engine, t, count);
	}

	void *CODECALL rawPtrFromGlobal(GlobalVar *global) {
		return global->rawDataPtr();
	}

	void CODECALL rawPtrWriteFilled(const void *ptr, Nat value) {
		if (!ptr)
			return;

		const GcType *t = runtime::gcTypeOf(ptr);
		if (t->kind == GcType::tArray) {
			((GcArray<byte> *)ptr)->filled = value;
		} else if (t->kind == GcType::tWeakArray) {
			((GcWeakArray<void *> *)ptr)->splatted(value);
		}
	}

	template <class T>
	void CODECALL rawPtrWrite(const void *ptr, Nat offset, T value) {
		const GcType *t = runtime::gcTypeOf(ptr);
		if (t->kind == GcType::tArray || t->kind == GcType::tWeakArray) {
			offset += OFFSET_OF(GcArray<Byte>, v);
		}

		*(T *)((byte *)ptr + offset) = value;
	}


	RawPtrType::RawPtrType(Engine &e) :
		Type(new (e) Str(S("RawPtr")), new (e) Array<Value>(), typeValue | typeFinal, Size::sPtr) {}

	Bool RawPtrType::loadAll() {
		Engine &e = engine;

		Value me(this, false);
		Array<Value> *rr = new (this) Array<Value>(2, Value(this, true));
		Array<Value> *rv = new (this) Array<Value>(2, Value(this, true));
		rv->at(1) = Value(this, false);
		Array<Value> *v = new (this) Array<Value>(1, Value(this, false));
		Array<Value> *vv = new (this) Array<Value>(2, Value(this, false));

		Value b(StormInfo<Bool>::type(e));
		Value n(StormInfo<Nat>::type(e));
		Value str(StormInfo<Str *>::type(e));

		add(inlinedFunction(e, Value(), Type::CTOR, rr, fnPtr(e, &rawPtrCopy))->makePure());
		add(inlinedFunction(e, Value(this, true), S("="), rv, fnPtr(e, &rawPtrAssign)));

		add(inlinedFunction(e, b, S("=="), vv, fnPtr(e, &numCmp<code::ifEqual>))->makePure());
		add(inlinedFunction(e, b, S("!="), vv, fnPtr(e, &numCmp<code::ifNotEqual>))->makePure());
		add(nativeFunction(e, n, S("hash"), v, address(&ptrHash)));
		add(nativeEngineFunction(e, str, S("toS"), v, address(&rawPtrToS)));
		add(inlinedFunction(e, b, S("empty"), v, fnPtr(e, &rawPtrEmpty))->makePure());
		add(inlinedFunction(e, b, S("any"), v, fnPtr(e, &rawPtrAny))->makePure());

		// Access.
		Array<Value> *vo = new (this) Array<Value>(2, Value(this, false));
		vo->at(1) = Value(StormInfo<Nat>::type(e));
		add(nativeFunction(e, Value(StormInfo<Bool>::type(e)), S("readBool"), vo, address(&rawPtrRead<Bool>)));
		add(nativeFunction(e, Value(StormInfo<Byte>::type(e)), S("readByte"), vo, address(&rawPtrRead<Byte>)));
		add(nativeFunction(e, Value(StormInfo<Int>::type(e)), S("readInt"), vo, address(&rawPtrRead<Int>)));
		add(nativeFunction(e, Value(StormInfo<Nat>::type(e)), S("readNat"), vo, address(&rawPtrRead<Nat>)));
		add(nativeFunction(e, Value(StormInfo<Long>::type(e)), S("readLong"), vo, address(&rawPtrRead<Long>)));
		add(nativeFunction(e, Value(StormInfo<Word>::type(e)), S("readWord"), vo, address(&rawPtrRead<Word>)));
		add(nativeFunction(e, Value(StormInfo<Float>::type(e)), S("readFloat"), vo, address(&rawPtrRead<Float>)));
		add(nativeFunction(e, Value(StormInfo<Double>::type(e)), S("readDouble"), vo, address(&rawPtrRead<Double>)));
		add(nativeFunction(e, me, S("readPtr"), vo, address(&rawPtrRead<void *>)));
		add(nativeFunction(e, Value(StormInfo<Nat>::type(e)), S("readCount"), v, address(&rawPtrCount)));
		add(nativeFunction(e, Value(StormInfo<Nat>::type(e)), S("readFilled"), v, address(&rawPtrFilled)));
		add(nativeFunction(e, Value(StormInfo<Nat>::type(e)), S("readSize"), v, address(&rawPtrSize)));

		Array<Value> *write = valList(e, 3, me, n, Value(StormInfo<Bool>::type(e)));
		add(nativeFunction(e, Value(), S("writeBool"), write, address(&rawPtrWrite<Bool>)));
		write = valList(e, 3, me, n, Value(StormInfo<Byte>::type(e)));
		add(nativeFunction(e, Value(), S("writeByte"), write, address(&rawPtrWrite<Byte>)));
		write = valList(e, 3, me, n, Value(StormInfo<Int>::type(e)));
		add(nativeFunction(e, Value(), S("writeInt"), write, address(&rawPtrWrite<Int>)));
		write = valList(e, 3, me, n, Value(StormInfo<Nat>::type(e)));
		add(nativeFunction(e, Value(), S("writeNat"), write, address(&rawPtrWrite<Nat>)));
		write = valList(e, 3, me, n, Value(StormInfo<Long>::type(e)));
		add(nativeFunction(e, Value(), S("writeLong"), write, address(&rawPtrWrite<Long>)));
		write = valList(e, 3, me, n, Value(StormInfo<Word>::type(e)));
		add(nativeFunction(e, Value(), S("writeWord"), write, address(&rawPtrWrite<Word>)));
		write = valList(e, 3, me, n, Value(StormInfo<Float>::type(e)));
		add(nativeFunction(e, Value(), S("writeFloat"), write, address(&rawPtrWrite<Float>)));
		write = valList(e, 3, me, n, Value(StormInfo<Double>::type(e)));
		add(nativeFunction(e, Value(), S("writeDouble"), write, address(&rawPtrWrite<Double>)));
		write = valList(e, 3, me, n, me);
		add(nativeFunction(e, Value(), S("writePtr"), write, address(&rawPtrWrite<void *>)));
		add(nativeFunction(e, Value(), S("writeFilled"), valList(e, 2, me, n), address(&rawPtrWriteFilled)));

		// Create from pointers:
		Value obj(StormInfo<Object>::type(e));
		Array<Value> *noPar = valList(e, 1, me.asRef());
		add(inlinedFunction(e, Value(), Type::CTOR, noPar, fnPtr(e, &rawPtrInitEmpty)));
		Array<Value> *objPar = valList(e, 2, me.asRef(), obj);
		add(inlinedFunction(e, Value(), Type::CTOR, objPar, fnPtr(e, &rawPtrInit)));
		Value tobj(StormInfo<TObject>::type(e));
		Array<Value> *tobjPar = valList(e, 2, me.asRef(), tobj);
		add(inlinedFunction(e, Value(), Type::CTOR, tobjPar, fnPtr(e, &rawPtrInit)));
		Value variant(StormInfo<Variant>::type(e));
		Array<Value> *variantPar = valList(e, 2, me.asRef(), variant.asRef());
		add(inlinedFunction(e, Value(), Type::CTOR, variantPar, fnPtr(e, &rawPtrCopy))); // Same as a copy of a RawPtr.
		add(nativeFunction(e, b, S("copyVariant"), valList(e, 2, me, variant.asRef()), address(&rawPtrCopyVariant)));

		// Inspect the data.
		Value type(StormInfo<Type *>::type(e));
		add(nativeFunction(e, wrapMaybe(type), S("type"), v, address(&rawPtrType)));
		add(nativeFunction(e, Value(StormInfo<Bool>::type(e)), S("isValue"), v, address(&rawPtrIsValue)));
		add(inlinedFunction(e, wrapMaybe(obj), S("asObject"), v, fnPtr(e, &rawPtrGet))->makePure());
		add(inlinedFunction(e, wrapMaybe(tobj), S("asTObject"), v, fnPtr(e, &rawPtrGet))->makePure());
		add(nativeFunction(e, variant, S("asVariant"), v, address(&DummyPtr::rawPtrVariant)));

		// Allocate an array containing some type.
		Array<Value> *typeNat = new (this) Array<Value>(2, StormInfo<Type *>::type(e));
		typeNat->at(1) = n;
		add(nativeFunction(e, me, S("allocArray"), typeNat, address(&rawPtrAllocArray))->make(fnStatic));

		// Read from a global variable.
		Array<Value> *globalVar = new (this) Array<Value>(1, StormInfo<GlobalVar *>::type(e));
		add(nativeFunction(e, me, S("fromGlobal"), globalVar, address(&rawPtrFromGlobal))->make(fnStatic));

		return Type::loadAll();
	}

	code::TypeDesc *RawPtrType::createTypeDesc() {
		return engine.ptrDesc();
	}

	void RawPtrType::modifyHandle(Handle *handle) {
		// We're basically a pointer, so use location based hashes!
		handle->locationHash = true;
	}

}
