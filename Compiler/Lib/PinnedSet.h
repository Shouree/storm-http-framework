#pragma once
#include "Core/Object.h"
#include "Core/Array.h"
#include "Gc/Gc.h"

namespace storm {
	STORM_PKG(core.unsafe);

	/**
	 * A set of pinned pointers.
	 *
	 * These pointers are scanned in the same way objects on the stack are scanned. This allows the
	 * pointers stored in here to refer to any part of any object in the system. This includes
	 * pointers into the middle of an object, and pointers to values that are located on the stack.
	 *
	 * It is then possible to query the set of pointers with a `core.unsafe.RawPtr` to see if this
	 * object contains a pointer to somewhere inside the specified `RawPtr`, and if so, at which
	 * offset. This can be done in O(log(n)), except for the first query that may require O(nlog(n))
	 * time. This is useful when collecting data from a running program to instrument memory
	 * accesses.
	 *
	 * While it is possible to query the data structure, it is *not* possible to retrieve individual
	 * pointers later on. This is because the pointers may refer to a stack that is no longer valid.
	 *
	 * This class is implemented as a flat array of objects that is sorted on demand.
	 */
	class PinnedSet : public Object {
		STORM_CLASS;
	public:
		// Create.
		STORM_CTOR PinnedSet();

		// Copy.
		PinnedSet(const PinnedSet &o);

		// Destroy.
		~PinnedSet();

		// Deep copy (does not do anything).
		virtual void STORM_FN deepCopy(CloneEnv *env);

		// Add a pointer to an object. The pointer may not necessarily point to the start of the object.
		void CODECALL put(void *ptr);

		// Check if an object is present in here. This pointer must be to the start of an
		// allocation. Returns true if this object contains a pointer to somewhere inside the
		// provided object. Note that the header of arrays are not counted as 'inside' the object
		// for consistency with offsets in RawPtr.
		Bool CODECALL has(const void *query);

		// Check if an offset into an object is present. 'query' should point to the start of an
		// allocation. Note: if 'query' is an array, 'offset' does not take the array header into
		// consideration, just like RawPtr would.
		Bool CODECALL has(const void *query, Nat offset);

		// Check if a range of addresses is present. 'query' should point to the start of an
		// allocation. Note: if 'query' is an array, 'offset' does not take the array header into
		// consideration, just like RawPtr would.
		Bool CODECALL has(const void *query, Nat offset, Nat size);

		// Same as 'has', but generates all offsets as an array.
		Array<Nat> *CODECALL offsets(const void *query);

		// Clear the set.
		void STORM_FN clear();

		// To string.
		virtual void STORM_FN toS(StrBuf *to) const;

	private:
		// Data.
		struct Data {
			// Number of elements.
			size_t count;

			// Number of filled elements.
			size_t filled;

			// Sorted?
			bool sorted;

			// Actual data.
			void *v[1];
		};

		// Data.
		UNKNOWN(PTR_NOGC) Data *data;

		// Root for the data.
		UNKNOWN(PTR_NOGC) Gc::Root *root;

		// Reserve size for at least 'n' elements.
		void reserve(size_t n);

		// Sort the array, and remove any duplicates (const, as it does not change the contained set).
		void sort() const;

		// Size of data struct for 'n' elements.
		static size_t dataSize(size_t n);
	};

	// Add the functions 'add', 'has' and 'offset' to PinnedSet, since they have parameter types
	// that are not present in C++.
	void addPinned(Type *rawPtr);

}
