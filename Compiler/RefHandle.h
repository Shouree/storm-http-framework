#pragma once
#include "Core/Handle.h"
#include "Code/Reference.h"
#include "Code/MemberRef.h"
#include "Code/Binary.h"

namespace storm {
	STORM_PKG(core.lang);

	class Function;

	/**
	 * A handle where all members are updated using references.
	 */
	class RefHandle : public Handle {
		STORM_CLASS;
	public:
		// Create. 'inside' is used to create references.
		STORM_CTOR RefHandle(code::Content *inside);

		// Set copy ctor.
		void STORM_FN setCopyCtor(code::Ref ref);
		void STORM_FN setCopyCtor(code::Binary *thunk);

		// Set destructor.
		void STORM_FN setDestroy(code::Ref ref);
		void STORM_FN setDestroy(code::Binary *thunk);

		// Set deep copy function.
		void STORM_FN setDeepCopy(code::Ref ref);
		void STORM_FN setDeepCopy(code::Binary *thunk);

		// Set to string function.
		void STORM_FN setToS(code::Ref ref);
		void STORM_FN setToS(code::Binary *thunk);

		// Set hash function.
		void STORM_FN setHash(code::Ref ref);
		void STORM_FN setHash(code::Binary *thunk);

		// Set equality function.
		void STORM_FN setEqual(code::Ref ref);
		void STORM_FN setEqual(code::Binary *thunk);

		// Set less-than function.
		void STORM_FN setLess(code::Ref ref);
		void STORM_FN setLess(code::Binary *thunk);

		// Set the type-info function.
		void STORM_FN setSerializedType(code::Ref ref);
		void STORM_FN setSerializedType(code::Binary *thunk);

	private:
		// Content to use when creating references.
		code::Content *content;

		// Ref to copy ctor (may be null).
		code::MemberRef *copyRef;

		// Ref to dtor (may be null).
		code::MemberRef *destroyRef;

		// Ref to deep copy fn.
		code::MemberRef *deepCopyRef;

		// Ref to toS fn.
		code::MemberRef *toSRef;

		// Ref to hash fn.
		code::MemberRef *hashRef;

		// Ref to equality fn.
		code::MemberRef *equalRef;

		// Ref to less-than fn.
		code::MemberRef *lessRef;

		// Ref to type-info fn.
		code::MemberRef *serializedTypeRef;
	};


	// Manual population of functions for some of the handles in the system. Will only fill in
	// function pointers, so fill in the proper references later on.
	void populateHandle(Type *type, Handle *h);

}
