#pragma once
#include "Size.h"
#include "Reference.h"
#include "Core/Array.h"
#include "Core/GcArray.h"
#include "Core/Object.h"
#include "Core/Exception.h"

namespace code {
	STORM_PKG(core.asm);

	namespace primitive {
		// Kind of primitives.
		enum PrimitiveKind {
			none,
			pointer,
			integer,
			real,
		};

		// C++ friendly name. Can not be used in function declarations visible to Storm.
		typedef PrimitiveKind Kind;

		// Get the name as a string.
		const wchar *name(Kind kind);
	}

	/**
	 * Description of a primitive type. Used inside 'TypeDesc'.
	 */
	class Primitive {
		STORM_VALUE;
	public:
		// Create a 'void' value.
		STORM_CTOR Primitive();

		// Create a primitive.
		STORM_CTOR Primitive(primitive::PrimitiveKind kind, Size size, Offset offset);

		// Get the kind.
		inline primitive::PrimitiveKind STORM_FN kind() const {
			return primitive::Kind((dataA & 0x1) | ((dataB & 0x1) << 1));
		}

		// Get the size (we do not preserve alignment).
		inline Size STORM_FN size() const {
			Nat s32 = (dataA & 0xFE) >> 1;
			Nat s64 = (dataB & 0xFE) >> 1;
			return Size(s32, s32, s64, s64);
		}

		// Get the offset.
		inline Offset STORM_FN offset() const {
			Nat o32 = (dataA & 0xFFFFFF00) >> 8;
			Nat o64 = (dataB & 0xFFFFFF00) >> 8;
			return Offset(o32, o64);
		}

		// Move to another offset.
		Primitive STORM_FN move(Offset to) const;

	private:
		// We store these fields inside 'dataA' and 'dataB'.
		// A0, B0: kind (A0 is LSB).
		// A1-7: 32-bit size
		// B1-7: 64-bit size
		// A8-31: 32-bit offset
		// B8-31: 64-bit offset
		Nat dataA;
		Nat dataB;
	};

	wostream &operator <<(wostream &to, const Primitive &p);
	StrBuf &STORM_FN operator <<(StrBuf &to, Primitive p);

	// Create primitive types easily.
	inline Primitive STORM_FN bytePrimitive() { return Primitive(primitive::integer, Size::sByte, Offset()); }
	inline Primitive STORM_FN intPrimitive() { return Primitive(primitive::integer, Size::sInt, Offset()); }
	inline Primitive STORM_FN ptrPrimitive() { return Primitive(primitive::pointer, Size::sPtr, Offset()); }
	inline Primitive STORM_FN longPrimitive() { return Primitive(primitive::integer, Size::sLong, Offset()); }
	inline Primitive STORM_FN floatPrimitive() { return Primitive(primitive::real, Size::sFloat, Offset()); }
	inline Primitive STORM_FN doublePrimitive() { return Primitive(primitive::real, Size::sDouble, Offset()); }

	/**
	 * Low-level description of a type. Instances of this class are used to tell the code backend
	 * enough about a type that is being passed as a parameter so that it can properly follow the
	 * calling convention of the system being targeted.
	 *
	 * A TypeDesc may describe the following categories of types:
	 * - A primitive type (size, floating point)
	 * - A complex type (size, constructor, destructor)
	 * - A simple type (size and offset off all members)
	 *
	 * A primitive type is some kind of number that can be handled by the CPU (eg. int, float). In
	 * this case, we store the total size of the type and what kind of number we're dealing with
	 * (ie. pointer, integer, floating point).
	 *
	 * A complex type is a composite type (ie. struct or class) that can not be trivially
	 * copied. Therefore, we are only concerned with the total size of the type and its copy
	 * constructor and destructor.
	 *
	 * A simple type is a composite type (ie. struct or class) that can be trivially copied. This
	 * means we do not need to concern ourselves with the copy constructor and destructor of the
	 * type, but we need to know all members of the type. If the simple type contains other simple
	 * types, these are also disassembled until only primitive types remain.
	 */
	class TypeDesc : public ObjectOn<Compiler> {
		STORM_CLASS;
	public:
		// Get the size of this type.
		virtual Size STORM_FN size() const;
	};

	class PrimitiveDesc : public TypeDesc {
		STORM_CLASS;
	public:
		STORM_CTOR PrimitiveDesc(Primitive p);

		Primitive v;

		virtual Size STORM_FN size() const { return v.size(); }
		void STORM_FN toS(StrBuf *to) const;
	};

	class ComplexDesc : public TypeDesc {
		STORM_CLASS;
	public:
		ComplexDesc(Size size, Ref ctor, Ref dtor);

		Size s;
		Ref ctor;
		Ref dtor;

		virtual Size STORM_FN size() const { return s; }
		void STORM_FN toS(StrBuf *to) const;
	};

	class SimpleDesc : public TypeDesc {
		STORM_CLASS;
	public:
		STORM_CTOR SimpleDesc(Size size, Nat entries);

		// Total size:
		Size s;

		// Primitives. These are assumed to be sorted according to their offset.
		GcArray<Primitive> *v;

		Primitive &STORM_FN at(Nat id) {
			if (id >= v->count)
				throw new (this) storm::ArrayError(id, Nat(v->count));
			return v->v[id];
		}
		Nat STORM_FN count() const {
			return Nat(v->count);
		}

		// Sort primitives.
		void STORM_FN sort();

		// virtual void STORM_FN deepCopy(CloneEnv *env);
		virtual Size STORM_FN size() const { return s; }
		void STORM_FN toS(StrBuf *to) const;
	};


	/**
	 * Helpers for creating TypeDesc objects.
	 */
	TypeDesc *STORM_FN voidDesc(EnginePtr e);
	TypeDesc *STORM_FN byteDesc(EnginePtr e);
	TypeDesc *STORM_FN intDesc(EnginePtr e);
	TypeDesc *STORM_FN ptrDesc(EnginePtr e);
	TypeDesc *STORM_FN longDesc(EnginePtr e);
	TypeDesc *STORM_FN floatDesc(EnginePtr e);
	TypeDesc *STORM_FN doubleDesc(EnginePtr e);

}
