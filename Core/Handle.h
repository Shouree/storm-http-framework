#pragma once
#include "Utils/Templates.h"
#include "Object.h"

namespace storm {
	STORM_PKG(core.lang); // TODO: Other package? If so, update Compiler/RefHandle as well.

	/**
	 * A type handle, ie. information about a type without actually knowing exactly which type it
	 * is. Used to make it easier to implement templates usable in Storm from C++.
	 *
	 * Note: we can always move objects with a memcpy (as this is what the gc does all the time). We
	 * may, however, need to make copies using the copy constructor.
	 *
	 * Note: all function pointers are exported to the GC, as they may point to generated code (which is gc:d).
	 */
	class Handle : public Object {
		STORM_CLASS;
	public:
		STORM_CTOR Handle();

		// Size of the type.
		size_t size;

		// GcType for arrays of the type.
		const GcType *gcArrayType;

		// Is this type hashed based off its pointer somehow?
		bool locationHash;

		// Copy constructor. Acts as an assignment (ie. never deeply copies heap-allocated types).
		typedef void (*CopyFn)(void *dest, const void *src);
		UNKNOWN(PTR_GC) CopyFn copyFn;

		// Safe copy. Falls back on memcpy.
		inline void safeCopy(void *dest, const void *src) const {
			if (copyFn)
				(*copyFn)(dest, src);
			else
				memcpy(dest, src, size);
		}

		// Destructor. May be null.
		typedef void (*DestroyFn)(void *obj);
		UNKNOWN(PTR_GC) DestroyFn destroyFn;

		// Helper for safe destroying. Zeroes out any used memory so that we do not retain any
		// unneeded objects.
		inline void safeDestroy(void *obj) const {
			if (destroyFn)
				(*destroyFn)(obj);
			memset(obj, 0, size);
		}

		// Deep copy an instance of this type. May be null.
		typedef void (*DeepCopyFn)(void *obj, CloneEnv *env);
		UNKNOWN(PTR_GC) DeepCopyFn deepCopyFn;

		// ToS implementation (always present).
		typedef void (*ToSFn)(const void *obj, StrBuf *to);
		UNKNOWN(PTR_GC) ToSFn toSFn;

		// Hash function.
		typedef Nat (*HashFn)(const void *obj);
		UNKNOWN(PTR_GC) HashFn hashFn;

		// Equality function.
		typedef Bool (*EqualFn)(const void *a, const void *b);
		UNKNOWN(PTR_GC) EqualFn equalFn;

		// Less-than function.
		typedef Bool (*LessFn)(const void *a, const void *b);
		UNKNOWN(PTR_GC) LessFn lessFn;

		// Helper for equality comparison. Works if one of 'equalFn' and 'lessFn' is implemented.
		inline Bool hasEqual() const {
			return equalFn || lessFn;
		}
		inline Bool equal(const void *a, const void *b) const {
			if (equalFn) {
				// Use '==' if possible.
				return (*equalFn)(a, b);
			} else {
				// Otherwise, we can use '!(a < b) && !(b < a)' instead.
				return !(*lessFn)(a, b) && !(*lessFn)(a, b);
			}
		}

		// Acquire information about serializing this type. May be null.
		typedef SerializedType *(*SerializedTypeFn)();
		UNKNOWN(PTR_GC) SerializedTypeFn serializedTypeFn;
	};

	/**
	 * Get limited type info for a type (may be pointer or reference).
	 */
	template <class T>
	struct StormInfo {
		// Type id in Storm for this module.
		static Nat id() {
			return BaseType<T>::Type::stormTypeId;
		}

		// Get a handle for T.
		static const Handle &handle(Engine &e) {
			return BaseType<T>::Type::stormHandle(e);
		}

		// Get the type for T.
		static Type *type(Engine &e) {
			return BaseType<T>::Type::stormType(e);
		}
	};

	template <>
	struct StormInfo<void> {
		static Nat id() {
			return -1;
		}

		static const Handle &handle(Engine &e) {
			return runtime::voidHandle(e);
		}

		static Type *type(Engine &e) {
			return null;
		}
	};

	// Specializations for built-in types. Generates StormInfo for them too, see Storm.h.
	STORM_PKG(core);

	/**
	 * Boolean value. Contains either 'true' or 'false'.
	 */
	STORM_PRIMITIVE(Bool, createBool);

	/**
	 * Unsigned 8-bit number.
	 *
	 * Can store values from 0 to 255. Operations yielding values that are out of range are
	 * truncated, effectively performing computations modulo 256.
	 */
	STORM_PRIMITIVE(Byte, createByte);

	/**
	 * Signed 32-bit number (using two's complement).
	 *
	 * Can store values from -2 147 843 648 to 2 147 843 647. Languages may assume that overflow
	 * does not occur (as C++ does). Currently, no language in Storm does this.
	 */
	STORM_PRIMITIVE(Int, createInt);

	/**
	 * Unsigned 32-bit number.
	 *
	 * Can store values from 0 to 4 294 967 295. Used as the standard type to describe quantities
	 * that are known to be positive, such as sizes of arrays etc. It is safe to assume that
	 * operations on this type are performed modulo 2^32.
	 */
	STORM_PRIMITIVE(Nat, createNat);

	/**
	 * Signed 64-bit number (using two's complement).
	 *
	 * Can store values from -9 223 372 036 854 775 808 to 9 223 372 036 854 775 807. Languages may
	 * assume that overflow does not occur (as C++ does). Currently, no language in Storm does this.
	 *
	 * Prefer to use Int over Long if possible, as the former is more efficient on most systems (32
	 * bit platforms requires additional instructions to manipulate 64 bit numbers, and 32-bit
	 * instructions are shorter on x86-64).
	 */
	STORM_PRIMITIVE(Long, createLong);

	/**
	 * Unsigned 64-bit number.
	 *
	 * Can store values from 0 to 18 446 744 073 709 551 615. It is safe to assume that operations
	 * on this type are performed modulo 2^64.
	 *
	 * Prefer to use Nat over Word if possible, as the former is more efficient on most systems (32
	 * bit platforms requires additional instructions to manipulate 64 bit numbers, and 32-bit
	 * instructions are shorter on x86-64).
	 */
	STORM_PRIMITIVE(Word, createWord);

	/**
	 * 32-bit floating point number.
	 */
	STORM_PRIMITIVE(Float, createFloat);

	/**
	 * 64-bit floating point number.
	 */
	STORM_PRIMITIVE(Double, createDouble);


	/**
	 * Helper for figuring out how to create objects.
	 */
	template <class T>
	struct CreateFn {
		// Create a value.
		static void fn(void *to, Engine &e) {
			new (Place(to)) T();
		}
	};

	template <class T>
	struct CreateFn<T *> {
		// Create an object or an actor.
		static void fn(void *to, Engine &e) {
			T **o = (T **)to;
			*o = new (e) T();
		}
	};
}
