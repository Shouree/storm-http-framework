#include "stdafx.h"
#include "Value.h"
#include "Type.h"
#include "Engine.h"
#include "Core/Str.h"
#include "Core/StrBuf.h"
#include "Utils/TypeInfo.h"

namespace storm {

	Value::Value() : type(null), ref(false) {}

	Value::Value(MAYBE(Type *) t) : type(t), ref(false) {}

	Value::Value(Type *t, Bool ref) : type(t), ref(ref) {}

	void Value::deepCopy(CloneEnv *env) {}

	Value Value::asRef() const {
		return Value(type, true);
	}

	Value Value::asRef(Bool ref) const {
		return Value(type, ref);
	}

	Bool Value::operator ==(Value o) const {
		// Treat null and null& the same.
		if (type == null && o.type == null)
			return true;
		return type == o.type
			&& ref == o.ref;
	}

	Bool Value::operator !=(Value o) const {
		return !(*this == o);
	}

	Bool Value::mayReferTo(MAYBE(Type *) o) const {
		if (type == null)
			return true; // void can refer to all types.
		if (o == null)
			return false; // no types may refer to void.
		return o->isA(type); // otherise: use inheritance
	}

	Bool Value::mayReferTo(Value v) const {
		// For objects: We can not create references from values.
		// For values, we need to be able to. In the future, maybe const refs from values?
		if (ref && !v.ref)
			if (isObject() || v.isObject())
				return false;
		return mayReferTo(v.type);
	}

	Bool Value::mayStore(MAYBE(Type *) o) const {
		if (type == o)
			return true;
		if (type == null) // note: implies o->type == null
			return false;
		// Only check inheritance for objects. We don't have to make sure that both are objects,
		// since 'isA' will fail if one is an object and another is a value.
		if ((type->typeFlags & typeClass) == typeClass)
			return o->isA(type);
		return false;
	}

	Bool Value::mayStore(Value v) const {
		if (ref != v.ref)
			return false;
		return mayStore(v.type);
	}

	Bool Value::matches(Value v, NamedFlags flags) const {
		if (flags & namedMatchNoInheritance)
			return type == v.type;
		else
			return mayReferTo(v);
	}

	Value common(Value a, Value b) {
		if (a.type == null || b.type == null)
			return Value();

		for (Type *t = a.type; t; t = t->super()) {
			if (b.type->isA(t))
				return Value(t);
		}
		return Value();
	}

	Value thisPtr(Type *t) {
		if ((t->typeFlags & typeValue) == typeValue)
			return Value(t, true);
		else
			return Value(t, false);
	}

	Size Value::size() const {
		if (!type) {
			return Size();
		} else if (ref) {
			return Size::sPtr;
		} else if (isValue()) {
			return type->size();
		} else {
			return Size::sPtr;
		}
	}

	static TypeKind::T convert(code::primitive::Kind k) {
		switch (k) {
		case code::primitive::none:
			return TypeKind::nothing;
		case code::primitive::pointer:
			return TypeKind::ptr;
		case code::primitive::integer:
			return TypeKind::signedNr;
		case code::primitive::real:
			return TypeKind::floatNr;
		}
		return TypeKind::nothing;
	}

	BasicTypeInfo Value::typeInfo() const {
		BasicTypeInfo::Kind kind = TypeKind::nothing;

		if (!type) {
			kind = TypeKind::nothing;
		} else if (ref) {
			kind = TypeKind::ptr;
		} else {
			code::TypeDesc *desc = type->typeDesc();
			if (code::PrimitiveDesc *p = as<code::PrimitiveDesc>(desc)) {
				kind = convert(p->v.kind());
			} else if (as<code::SimpleDesc>(desc)) {
				kind = TypeKind::userTrivial;
			} else if (as<code::ComplexDesc>(desc)) {
				kind = TypeKind::userComplex;
			} else {
				assert(false);
			}
		}

		BasicTypeInfo r = {
			size().current(),
			kind,
		};
		return r;
	}

	code::TypeDesc *Value::desc(Engine &e) const {
		if (!type)
			return e.voidDesc();
		if (ref)
			return e.ptrDesc();
		return type->typeDesc();
	}

	code::TypeDesc *desc(EnginePtr e, Value v) {
		return v.desc(e.v);
	}

	Bool Value::returnInReg() const {
		if (ref)
			return true;
		if (!type)
			return true;
		return isAsmType();
	}

	Bool Value::isValue() const {
		if (!type)
			return false;
		return (type->typeFlags & typeValue) == typeValue;
	}

	Bool Value::isObject() const {
		if (!type)
			return false;
		return (type->typeFlags & typeClass) == typeClass;
	}

	Bool Value::isClass() const {
		if (!isObject())
			return false;
		return type->isA(StormInfo<Object>::type(type->engine));
	}

	Bool Value::isActor() const {
		if (!isObject())
			return false;
		return type->isA(StormInfo<TObject>::type(type->engine));
	}

	Bool Value::isPrimitive() const {
		if (!type)
			return false;

		if (type->typeFlags & typeClass)
			return false;

		// Check if it is a primitive.
		code::PrimitiveDesc *desc = as<code::PrimitiveDesc>(type->typeDesc());
		if (!desc)
			return false;

		// Exclude pointers, those need to be treated differently.
		return desc->v.kind() != code::primitive::pointer;
	}

	Bool Value::isAsmType() const {
		if (!type)
			return false;

		if (isObject())
			return true;

		// If it is a primitive type.
		if (as<code::PrimitiveDesc>(type->typeDesc()))
			return true;

		return false;
	}

	Bool Value::isPtr() const {
		if (!type)
			return false;
		if ((type->typeFlags & typeClass) == typeClass)
			return true;
		if (code::PrimitiveDesc *desc = as<code::PrimitiveDesc>(type->typeDesc()))
			return desc->v.kind() == code::primitive::pointer;
		return false;
	}

	code::Operand Value::copyCtor() const {
		if (isValue() && !isPrimitive()) {
			Function *ctor = type->copyCtor();
			if (!ctor) {
				WARNING(L"Not returning a proper copy-constructor!");
				return code::Operand();
			}

			return code::Ref(ctor->ref());
		} else {
			return code::Operand();
		}
	}

	code::Operand Value::destructor() const {
		if (isValue() && !isPrimitive()) {
			if (Function *dtor = type->destructor())
				return dtor->ref();
		}

		return code::Operand();
	}

	StrBuf &operator <<(StrBuf &to, Value v) {
		if (v.type) {
			to << v.type->identifier();
			if (v.ref)
				to << S("&");
		} else {
			to << S("void");
		}
		return to;
	}

	wostream &operator <<(wostream &to, const Value &v) {
		if (v.type) {
			to << v.type->identifier()->c_str();
			if (v.ref)
				to << L"&";
		} else {
			to << L"void";
		}
		return to;
	}

#ifdef VISUAL_STUDIO
	Array<Value> *valList(Engine &e, nat count, ...) {
		Array<Value> *r = new (e) Array<Value>();
		r->reserve(count);

		va_list l;
		va_start(l, count);

		for (nat i = 0; i < count; i++) {
			r->push(va_arg(l, Value));
		}

		va_end(l);
		return r;
	}
#endif

}
