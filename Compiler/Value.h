#pragma once
#include "Core/EnginePtr.h"
#include "Code/Operand.h"
#include "Code/TypeDesc.h"
#include "NamedFlags.h"

namespace storm {
	STORM_PKG(core.lang);

	/**
	 * Describes a value inside Storm.
	 *
	 * TODO: Think about thread safety here.
	 */
	class Value {
		STORM_VALUE;
	public:
		// Create 'void'.
		STORM_CTOR Value();

		// Create from a specific type. Never a reference.
		STORM_CAST_CTOR Value(MAYBE(Type *) type);

		// Create from a type and if it is to be a reference.
		STORM_CTOR Value(Type *type, Bool ref);

		// Deep copy.
		void deepCopy(CloneEnv *env);

		// Type we're referring to.
		MAYBE(Type*) type;

		// Reference type?
		Bool ref;

		// Make into a reference.
		Value STORM_FN asRef() const;
		Value STORM_FN asRef(Bool ref) const;

		// Same type?
		Bool STORM_FN operator ==(Value o) const;
		Bool STORM_FN operator !=(Value o) const;

		// Can this value store a type of 'x'?
		Bool STORM_FN canStore(MAYBE(Type *) x) const;
		Bool STORM_FN canStore(Value v) const;

		// Does this value match another value according to NamedFlags? Note that this relation is not reflexive.
		// If no special flags are set, then it is equivalent to 'canStore'.
		Bool STORM_FN matches(Value v, NamedFlags flags) const;

		/**
		 * Code generation information.
		 */

		// Return this type in a register? We count 'void' as being returned in a register.
		Bool STORM_FN returnInReg() const;

		// Is this a value-type or an object-type (that is heap-allocated)? Either 'isValue' or
		// 'isObject' returns true, except for 'void', where none returns true.
		Bool STORM_FN isValue() const;
		Bool STORM_FN isObject() const;

		// Is this a class- or an actor type? One of these return true if 'isObject' returns true.
		Bool STORM_FN isClass() const;
		Bool STORM_FN isActor() const;

		// Is this a primitive type? e.g. int, float, etc. These do not need explicit construction
		// and can be stored in registers. Note: Object-types and references are not primitives in
		// this regard, as pointers generally need initialization. 'void' is not considered a
		// primitive.
		Bool STORM_FN isPrimitive() const;

		// Can this type be manipulated by the CPU directly. This includes the primitive types, but
		// also reference types and object types. ASM types never have destructors. Does not
		// consider references to be AsmTypes, as they are expected to be handled at a different level.
		Bool STORM_FN isAsmType() const;

		// Is this some kind of pointer that could need garbage collection?
		Bool STORM_FN isPtr() const;

		// The size of this type.
		Size STORM_FN size() const;

		// Get type info for this type.
		BasicTypeInfo typeInfo() const;

		// Get a type description of this type.
		code::TypeDesc *desc(Engine &e) const ON(Compiler);

		/**
		 * Access to common member functions.
		 */

		// Get an Operand pointing to the copy constructor for this type. Only returns something
		// useful for value types, as other types can and shall be copied using an inline
		// mov-instruction.
		code::Operand STORM_FN copyCtor() const ON(Compiler);

		// Get an Operand pointing to the destructor for this type. May return Operand(), meaning no
		// destructor is needed.
		code::Operand STORM_FN destructor() const ON(Compiler);
	};

	// Get a description of this type.
	code::TypeDesc *STORM_FN desc(EnginePtr e, Value v) ON(Compiler);


	/**
	 * Compute the common denominator of two values so that
	 * it is possible to cast both 'a' and 'b' to the resulting
	 * type. In case 'a' and 'b' are unrelated, Value() - void
	 * is returned.
	 */
	Value STORM_FN common(Value a, Value b) ON(Compiler);

	// Create a this pointer for a type.
	Value STORM_FN thisPtr(Type *t) ON(Compiler);

	// Output.
	wostream &operator <<(wostream &to, const Value &v);
	StrBuf &STORM_FN operator <<(StrBuf &to, Value v);

	// Generate a list of values.
#ifdef VISUAL_STUDIO
	// Uses a visual studio specific extension...
	Array<Value> *valList(Engine &e, Nat count, ...);
#elif defined(USE_VA_TEMPLATE)
	template <class... Val>
	void valListAdd(Array<Value> *to, const Value &first, const Val&... rest) {
		to->push(first);
		valListAdd(to, rest...);
	}

	inline void valListAdd(Array<Value> *to) {
		UNUSED(to);
	}

	template <class... Val>
	Array<Value> *valList(Engine &e, Nat count, const Val&... rest) {
		assert(sizeof...(rest) == count, L"'count' does not match the number of parameters passed to 'valList'.");
		Array<Value> *v = new (e) Array<Value>();
		v->reserve(count);

		valListAdd(v, rest...);

		return v;
	}
#else
#error "Can not implement 'valList'"
#endif
}
