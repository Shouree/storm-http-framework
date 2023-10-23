#pragma once
#include "RootObject.h"

namespace storm {
	STORM_PKG(core);

	class Engine;
	class Type;
	class Str;
	struct GcType;

	/**
	 * The Object root class. This is the base class of all objects allocated on the heap. As we're
	 * using a garbage collector, destructors of heap-allocated objects are not called promptly if
	 * at all. Destructors are called if one of the following conditions are satisfied:
	 *
	 * - A destructor has been *explicitly declared* in a class or any of its base- or derived classes.
	 * - A destructor has been *explicitly declared* in a member, which is stored by value.
	 */
	class Object : public STORM_HIDDEN(RootObject) {
		STORM_CLASS;
	public:
		// Default constructor.
		STORM_CTOR Object();

		/**
		 * Members common to all objects.
		 */

		// Deep copy of all objects in here.
		virtual void STORM_FN deepCopy(CloneEnv *env);

		// Convert to string.
		virtual Str *STORM_FN toS() const;
		virtual void STORM_FN toS(StrBuf *to) const;
	};

	// Do the two objects `a` and `b` have the same dynamic type?
	inline Bool STORM_FN sameType(const Object *a, const Object *b) {
		return runtime::typeOf(a) == runtime::typeOf(b);
	}
}

