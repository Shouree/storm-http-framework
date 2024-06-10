#pragma once
#include "Core/EnginePtr.h"
#include "Code/Operand.h"
#include "Code/TypeDesc.h"
#include "NamedFlags.h"

namespace storm {
	STORM_PKG(core.lang);

	/**
	 * Represents a variable that contains a value of a particular type.
	 *
	 * As such, a value is a pointer to a type alongside whether the variable contains the variable
	 * directly, or if it is a reference to the variable. This is mostly relevant for function
	 * parameters, but is often useful for local variables as well.
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

		// The type stored. If `null`, the value refers to `void` and the value in `ref` is meaningless.
		MAYBE(Type*) type;

		// Is this value representing a reference to `type`?
		Bool ref;

		// Any/empty. Empty means 'void'.
		Bool STORM_FN any() { return type != null; }
		Bool STORM_FN empty() { return type == null; }

		// Return a copy that is a reference.
		Value STORM_FN asRef() const;
		Value STORM_FN asRef(Bool ref) const;

		// Same type?
		Bool STORM_FN operator ==(Value o) const;
		Bool STORM_FN operator !=(Value o) const;

		// Output.
		void STORM_FN toS(StrBuf *to) const;

		// Check if a value of this type may refer to a value of type `x`. As the name implies, this
		// check is performed with references in mind. As such, this is used to determine if formal
		// parameters of a function matches actual parameters, or when figuring out if a (copy)
		// assignment should be allowed.
		//
		// Note: `void` is considered to be able to refer to all other types.
		//
		// Note: This definition is separate from storage. Since the size of derived value types
		// differ, it is not possible to store a derived type in a variable sized after the
		// super type (without slicing), even if the super type may refer to the derived type.
		//
		// Note: This function was previously named 'canStore', but was split due to to the possible
		// confusion with storage size.
		Bool STORM_FN mayReferTo(MAYBE(Type *) x) const;
		Bool STORM_FN mayReferTo(Value x) const;

		// See if it is possible to store a value of `x` inside a value of this type without any
		// conversions. Works like 'mayReferTo', except that value types use strict equality, and
		// `void` is not considered to be able to store any other types.
		Bool STORM_FN mayStore(MAYBE(Type *) x) const;
		Bool STORM_FN mayStore(Value x) const;

		// Does this value match another value according to NamedFlags? Works like 'mayReferTo',
		// but flags effect the outcome. Note that this relation is not reflexive. If no special
		// flags are set, then it is equivalent to 'mayReferTo'.
		Bool STORM_FN matches(Value v, NamedFlags flags) const;

		/**
		 * Code generation information.
		 */

		// Should this type be returned in a register? Note: we count `void` as being returned in a register.
		Bool STORM_FN returnInReg() const;

		// Does this value refer to a value-type? Either `isValue` or `isObject` returns true for non-void types.
		Bool STORM_FN isValue() const;

		// Does this value refer to a heap-allocated type (i.e. a class or an actor)? Either
		// `isValue` or `isObject` returns true for non-void types.
		Bool STORM_FN isObject() const;

		// Is this a class type?
		Bool STORM_FN isClass() const;

		// Is this an actor type?
		Bool STORM_FN isActor() const;

		// Is this a primitive type? e.g. int, float, etc. These do not need explicit construction
		// and can be stored in registers. Note: Object-types and references are not primitives in
		// this regard, as pointers generally need initialization. `void` is not considered a
		// primitive.
		Bool STORM_FN isPrimitive() const;

		// Can this type be manipulated by the CPU directly. This includes the primitive types, but
		// also reference types and object types. ASM types never have destructors. Does not
		// consider references to be AsmTypes, as they are expected to be handled at a different level.
		Bool STORM_FN isAsmType() const;

		// Is this some kind of pointer that may need garbage collection?
		Bool STORM_FN isPtr() const;

		// Return the size of this type. Respects by-reference semantics and the `ref` member. As
		// such, always return a pointer size for classes and actors.
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
