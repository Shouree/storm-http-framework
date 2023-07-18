#pragma once
#include "Object.h"
#include "TObject.h"
#include "Handle.h"

namespace storm {
	STORM_PKG(core);

	/**
	 * A set of weak references for use in Storm and C++.
	 *
	 * This hash set only manages references to actor objects (ie. TObjects), as other objects are
	 * usually not relevant to keep weak references to (as they are copied). This simplifies the
	 * implementation of this set a great deal as well.
	 *
	 * The implementation is inspired from the hash map implementation found in Lua, see Map.h and
	 * Set.h for details.
	 *
	 * Note: Under MPS, the references stored in here may be to objects that have been finalized.
	 */

	/**
	 * Base class, type agnostic.
	 */
	class WeakSetBase : public Object {
		STORM_CLASS;
	public:
		// Empty set.
		WeakSetBase();

		// Copy another set.
		WeakSetBase(const WeakSetBase &other);

		// Deep copy.
		virtual void STORM_FN deepCopy(CloneEnv *env);

		/**
		 * Non-generic public interface.
		 */

		// Any and empty. Note: this is just an indication. If empty() returns true, then there is
		// definitely nothing in the set, but otherwise we can not be sure.
		inline Bool STORM_FN any() const { return size > 0; }
		inline Bool STORM_FN empty() const { return size == 0; }

		// Clear.
		void STORM_FN clear();

		// Shrink to fit the contained entries as tightly as possible.
		void STORM_FN shrink();

		// To string.
		virtual void STORM_FN toS(StrBuf *to) const;

		/**
		 * Low-level operations.
		 */

		// Put a value. Returns 'true' if the value did not exist before.
		Bool CODECALL putRaw(TObject *key);

		// Contains?
		Bool CODECALL hasRaw(TObject *key);

		// Note: we're missing 'get' and 'at', as they are useless for TObjects.

		// Remove a value from the set.
		Bool CODECALL removeRaw(TObject *key);

		/**
		 * Debug and benchmarking functions.
		 */

		// Count the number of collisions.
		Nat STORM_FN countCollisions() const;

		// Find the longest chain.
		Nat STORM_FN countMaxChain() const;

		// Get the current capacity.
		inline Nat STORM_FN capacity() const { return info ? info->count : 0; }

	private:
		// Number of slots that are currently occupied. This may decrease during the next rehash.
		Nat size;

		// Minimum capacity.
		static const nat minCapacity;

		// Gc-type for the info.
		static const GcType infoType;

		// Slot information.
		struct Info {
			// Used? Part of a chain?
			nat status;

			// Status codes (at the end of the nat interval to not interfere).
			static const nat free = -1;
			static const nat end = -2;

			// Cached hash value.
			nat hash;
		};

		// Allocated memory.
		GcArray<Info> *info;
		GcWeakArray<TObject> *data;

		// Watch out for moving objects.
		GcWatch *watch;

		// Allocate data for a specific capacity. Assumes 'info', 'key' and 'value' are null.
		void alloc(nat capacity);

		// Allocate data, assuming we are re-hashing due to stale location dependency.
		void allocRehash(nat capacity);

		// Grow (if needed) to fit at least one more element.
		void grow();

		// Do a re-hash to a specific size (asssumed to be power of two).
		void rehash(nat size);

		// Do a re-hash while looking for an element. Assumes 'watch' is non-null, and that some object have moved.
		nat rehashFind(nat size, TObject *key);

		// Do a re-hash while removing an element. Assumes 'watch' is non-null, and that some object have moved.
		bool rehashRemove(nat size, TObject *key);

		// Insert a node, given its hash is known (eg. when re-hashing). Assumes no other node with
		// the same key exists, and will therefore always insert the element. Returns the slot
		// inserted into. 'watch' is a slot that needs to be updated whenever a slot is moved.
		nat insert(TObject *key, nat hash, nat &watch);

		// Remove an element, ignoring any moved objects. Returns 'true' if an object was removed.
		bool remove(TObject *key);

		// Find the current location of 'key', given 'hash'. Returns 'Info::free' if none exists.
		nat findSlot(TObject *key, nat hash);

		// Helper for 'findSlot'. Does not work properly if objects have moved.
		nat findSlotI(TObject *key, nat hash);

		// Compute the primary slot for a node, given its hash.
		nat primarySlot(nat hash) const;

		// Find a free slot. Always succeeds as long as size != capacity.
		nat freeSlot();

		// Last seen free slot in this table. Used by 'freeSlot'.
		nat lastFree;

		// Clean splatted references if needed.
		void clean();

		// Helper for copying arrays.
		GcArray<Info> *copyArray(const GcArray<Info> *src);
		GcWeakArray<TObject> *copyArray(const GcWeakArray<TObject> *src);

	public:

		/**
		 * Iterator.
		 *
		 * This iterator is a bit special due to the nature of weak references. This iterator can be
		 * asked about the next element, and will return elements until there are no more. As
		 * elements may disappear without notice, it is meaningless to express ranges of elements,
		 * as one or more of the iterators may become invalid at any point.
		 *
		 * Note: since reading an element is potentially a destructive operation (we may have to
		 * rehash the table due to moved entries), we keep pointers to the data from the set in here
		 * so that iterators do not break when that happens.
		 */
		class Iter {
			STORM_VALUE;
		public:
			// Create empty iterator.
			Iter();

			// Pointing to the first element.
			Iter(WeakSetBase *owner);

			// Get the next element, or null if at the end.
			MAYBE(TObject *) CODECALL nextRaw();

		private:
			// The gc array from the set.
			GcWeakArray<TObject> *data;

			// Current position.
			Nat pos;
		};

		// Raw iterator.
		Iter CODECALL iterRaw();

		// Friend.
		friend Iter;
	};

	// Let Storm know about the WeakSet template.
	STORM_TEMPLATE(WeakSet, createWeakSet);

	/**
	 * C++ interface.
	 *
	 * TODO: Set vtables for class in constructors.
	 */
	template <class K>
	class WeakSet : public WeakSetBase {
		STORM_SPECIAL;
	public:
		// Get the Storm type for this object.
		static Type *stormType(Engine &e) {
			return runtime::cppTemplate(e, WeakSetId, 1, StormInfo<K>::id());
		}

		// Empty set.
		WeakSet() : WeakSetBase() {
			runtime::setVTable(this);
		}

		// Copy set.
		WeakSet(WeakSet<K> *o) : WeakSetBase(o) {
			runtime::setVTable(this);
		}

		// Insert a value. Returns 'true' if the value did not exist before.
		Bool put(K *k) {
			return putRaw(k);
		}

		// Contains a key?
		Bool has(K *k) {
			return hasRaw(k);
		}

		// Remove a key.
		bool remove(K *k) {
			return removeRaw(k);
		}

		/**
		 * Iterator.
		 */
		class Iter : public WeakSetBase::Iter {
		public:
			Iter() : WeakSetBase::Iter() {}

			Iter(WeakSet<K> *owner) : WeakSetBase::Iter(owner) {}

			MAYBE(K *) next() {
				return (K *)nextRaw();
			}
		};

		// Create an interator.
		Iter iter() {
			return Iter(this);
		}
	};

}
