#pragma once
#include "Object.h"
#include "Handle.h"

namespace storm {
	STORM_PKG(core);

	/**
	 * Set for use in Storm and in C++.
	 *
	 * Note: actors are currently supported by disallowing custom hash functions being associated
	 * with them, and always using their address as the value for hashing.
	 *
	 * The implementation is inspired from the hash map implementation found in Lua. All keys are
	 * stored as a flat array. Chaining is done by maintaining pointers in each of the slots of the
	 * array. An entry is said to be in its primary position if it is in the location computed by
	 * the hash function. This implementation maintains the invariant that for each hash value, the
	 * element contained is either the element in the primary position, or that hash does not exist
	 * in the hashmap.
	 *
	 * Special quirks when using a moving Gc with pointer-based hashes: as objects may be moved and
	 * thus pointers may be updated at any point, the hash map must be aware of this and act
	 * accordingly.
	 */


	/**
	 * The base class used in Storm. Use derived Set<> in C++.
	 */
	class SetBase : public Object {
		STORM_CLASS;
	public:
		// Empty set.
		SetBase(const Handle &key);

		// Copy another set.
		SetBase(const SetBase &other);

		// Deep copy.
		virtual void STORM_FN deepCopy(CloneEnv *env);

		// Key and value handle.
		const Handle &keyT;

		/**
		 * Non-generic public interface.
		 */

		// # of contained elements.
		inline Nat STORM_FN count() const { return size; }

		// Any elements?
		inline Bool STORM_FN any() const { return size > 0; }

		// Empty?
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

		// Put a value.
		Bool CODECALL putRaw(const void *key);

		// Put values from a set.
		void CODECALL putSetRaw(SetBase *src);

		// Contains value?
		Bool CODECALL hasRaw(const void *key);

		// Get the key previously stored which is considered equal to 'key'. Throws if it does not exist.
		void *CODECALL getRaw(const void *key);

		// Get a key considered equal to 'key', or insert and return 'key' if none exists.
		void *CODECALL atRaw(const void *key);

		// Remove a value. Returns 'true' if we found one to remove.
		Bool CODECALL removeRaw(const void *key);

		/**
		 * Benchmarking functions. Usually slow, only intended as a way of exploring performance
		 * bottlenecks.
		 */

		// Count the number of collisions.
		Nat STORM_FN countCollisions() const;

		// Find the longest chain.
		Nat STORM_FN countMaxChain() const;

		// Print the low-level layout.
		void dbg_print();

	private:
		// # of contained elements.
		Nat size;

		// Minimum capacity.
		static const nat minCapacity;

		// Gc-type for the Info.
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

		// Allocated memory. Split into two regions: info and key. Each of these are the same number
		// of elements in size.
		GcArray<Info> *info;
		GcArray<byte> *key;

		// Watch if objects move (if needed).
		GcWatch *watch;

		// Our capacity.
		inline nat capacity() const { return info ? nat(info->count) : 0; }

		// Get locations for keys or values.
		inline void *keyPtr(nat id) { return key->v + id*keyT.size; }
		inline const void *keyPtr(nat id) const { return key->v + id*keyT.size; }

		inline void *keyPtr(GcArray<byte> *a, nat id) { return a->v + id*keyT.size; }

		// Allocate data for a specific capacity. Assumes 'info', 'key' and 'value' are null.
		void alloc(nat capacity);

		// Allocate data for capacity, assumes we are re-hashing.
		void allocRehash(nat capacity);

		// Grow (if needed) to fit at least one more element.
		void grow();

		// Do a re-hash to a specific size (asssumed to be power of two).
		void rehash(nat size);

		// Do a re-hash while looking for an element. Assumes 'watch' is non-null, and that some object have moved.
		nat rehashFind(nat size, const void *key);

		// Do a re-hash while removing an element. Assumes 'watch' is non-null, and that some object have moved.
		bool rehashRemove(nat size, const void *key);

		// Compute the hash for an element, taking into account the required location
		// dependency. Use only when inserting things, as we will then falsly react whenever that
		// object is moved.
		nat newHash(const void *key);

		// Check if the hash of an object has changed due to the GC moving the object. Only returns
		// 'true' if we're using location dependent hashes with the 'watch' object.
		bool changedHash(const void *key);

		// Insert a node, given its hash is known (eg. when re-hashing). Assumes no other node with
		// the same key exists, and will therefore always insert the element. Returns the slot
		// inserted into. 'watch' is a slot that needs to be updated whenever a slot is moved.
		nat insert(const void *key, nat hash, nat &watch);

		// Remove an element, ignoring any moved objects. Returns 'true' if an object was removed.
		bool remove(const void *key);

		// Find the current location of 'key', given 'hash'. Returns 'Info::free' if none exists.
		nat findSlot(const void *key, nat hash);

		// Helper for 'findSlot'. Does not work properly if objects have moved.
		nat findSlotI(const void *key, nat hash);

		// Compute the primary slot for a node, given its hash.
		nat primarySlot(nat hash) const;

		// Find a free slot. Always succeeds as long as size != capacity.
		nat freeSlot();

		// Last seen free slot in this table. Used by 'freeSlot'.
		nat lastFree;

		// Helper for copying arrays.
		GcArray<Info> *copyArray(const GcArray<Info> *src);
		GcArray<byte> *copyArray(const GcArray<byte> *src, const GcArray<Info> *info, const Handle &type);

	public:

		/**
		 * Iterator.
		 *
		 * Note: since reading an element is potentially a destructive operation (we may have to
		 * rehash the table due to moved entries), we keep pointers to the data from the set in here
		 * so that iterators do not break when that happens.
		 */
		class Iter {
			STORM_VALUE;
		public:
			// Pointing to the end.
			Iter();

			// Pointing to the first element.
			Iter(SetBase *owner);

			// Compare.
			bool CODECALL operator ==(const Iter &o) const;
			bool CODECALL operator !=(const Iter &o) const;

			// Advance.
			Iter &operator ++();
			Iter operator ++(int z);

			// Raw get functions.
			void *rawVal() const;

			// Raw pre- and post increment.
			Iter &CODECALL preIncRaw();
			Iter CODECALL postIncRaw();

			inline Nat p() { return pos; }
		private:
			// The two gc arrays from the set.
			// TODO: as the key and value arrays will eventually need to contain information about
			// which elements are free, we can elliminate 'info' eventually.
			GcArray<Info> *info;
			GcArray<byte> *key;

			// Current position.
			Nat pos;

			// At end?
			bool atEnd() const;
		};

		// Raw begin and end.
		Iter CODECALL beginRaw();
		Iter CODECALL endRaw();

		// Friend.
		friend Iter;
	};

	// Let Storm know about the Set template.
	STORM_TEMPLATE(Set, createSet);

	/**
	 * C++ interface.
	 */
	template <class K>
	class Set : public SetBase {
		STORM_SPECIAL;
	public:
		// Get the Storm type for this object.
		static Type *stormType(Engine &e) {
			return runtime::cppTemplate(e, SetId, 1, StormInfo<K>::id());
		}

		// Empty set.
		Set() : SetBase(StormInfo<K>::handle(engine())) {
			runtime::setVTable(this);
		}

		// Copy set.
		Set(const Set<K> &o) : SetBase(o) {
			runtime::setVTable(this);
		}

		// Insert a value into the set, or update the existing one. Returns 'true' if it did not exist before.
		Bool put(const K &k) {
			return putRaw(&k);
		}

		// Insert values from another set into this one.
		void put(Set<K> *from) {
			putSetRaw(from);
		}

		// Contains a key?
		Bool has(const K &k) {
			return hasRaw(&k);
		}

		// Get a previously inserted key. Throws if not found.
		K &get(const K &k) {
			return *(K *)getRaw(&k);
		}

		// Get a previously inserted key. Inserts and returns 'k' if not found.
		K &at(const K &k) {
			return *(K *)atRaw(&k);
		}

		// Remove a value.
		Bool remove(const K &k) {
			return removeRaw(&k);
		}

		/**
		 * Iterator.
		 */
		class Iter : public SetBase::Iter {
		public:
			Iter() : SetBase::Iter() {}
			Iter(Set<K> *owner) : SetBase::Iter(owner) {}

			K operator *() const {
				return *(K *)rawVal();
			}

			// We're using 'v' as a lone 'k' is meaningless to Storm.
			const K &v() const {
				return *(const K *)rawVal();
			}
		};

		// Create the iterator.
		Iter begin() {
			return Iter(this);
		}

		Iter end() {
			return Iter();
		}
	};


	// Let Storm know about the Set template.
	STORM_TEMPLATE(RefSet, createRefSet);

	/**
	 * C++ interface.
	 */
	template <class K>
	class RefSet : public SetBase {
		STORM_SPECIAL;
	public:
		// Get the Storm type for this object.
		static Type *stormType(Engine &e) {
			return runtime::cppTemplate(e, RefSetId, 1, StormInfo<K>::id());
		}

		// Empty set.
		RefSet() : SetBase(runtime::refObjHandle(engine())) {
			runtime::setVTable(this);
		}

		// Copy set.
		RefSet(const RefSet<K> &o) : SetBase(o) {
			runtime::setVTable(this);
		}

		// Insert a value into the set, or update the existing one. Returns 'true' if it did not exist before.
		Bool put(const K &k) {
			return putRaw(&k);
		}

		// Insert values from another set into this one.
		void put(RefSet<K> *from) {
			putSetRaw(from);
		}

		// Contains a key?
		Bool has(const K &k) {
			return hasRaw(&k);
		}

		// Get a previously inserted key. Throws if not found.
		K &get(const K &k) {
			return *(K *)getRaw(&k);
		}

		// Get a previously inserted key. Inserts and returns 'k' if not found.
		K &at(const K &k) {
			return *(K *)atRaw(&k);
		}

		// Remove a value.
		Bool remove(const K &k) {
			return removeRaw(&k);
		}

		/**
		 * Iterator.
		 */
		class Iter : public SetBase::Iter {
		public:
			Iter() : SetBase::Iter() {}
			Iter(RefSet<K> *owner) : SetBase::Iter(owner) {}

			K operator *() const {
				return *(K *)rawVal();
			}

			// We're using 'v' as a lone 'k' is meaningless to Storm.
			const K &v() const {
				return *(const K *)rawVal();
			}
		};

		// Create the iterator.
		Iter begin() {
			return Iter(this);
		}

		Iter end() {
			return Iter();
		}
	};
}
