#pragma once
#include "Object.h"
#include "Handle.h"

namespace storm {
	STORM_PKG(core);

	/**
	 * Map for use in Storm and in C++.
	 *
	 * Note: actors are currently supported by disallowing custom hash functions being associated
	 * with them, and always using their address as the value for hashing.
	 *
	 * The implementation is inspired from the hash map implementation found in Lua. All keys and
	 * values are stored as a flat array. Chaining is done by maintaining pointers in each of the
	 * slots of the array. An entry is said to be in its primary position if it is in the location
	 * computed by the hash function. This implementation maintains the invariant that for each hash
	 * value, the element contained is either the element in the primary position, or that hash does
	 * not exist in the hash map.
	 *
	 * Special quirks when using a moving Gc with pointer-based hashes: as objects may be moved and
	 * thus pointers may be updated at any point, the hash map must be aware of this and act
	 * accordingly.
	 */


	/**
	 * The base class used in Storm. Use derived Map<> in C++.
	 */
	class MapBase : public Object {
		STORM_CLASS;
	public:
		// Empty map.
		MapBase(const Handle &key, const Handle &value);

		// Copy another map.
		MapBase(const MapBase &other);

		// Deep copy.
		virtual void STORM_FN deepCopy(CloneEnv *env);

		// Key and value handle.
		const Handle &keyT;
		const Handle &valT;

		/**
		 * Non-generic public interface.
		 */

		// # of contained elements.
		inline Nat STORM_FN count() const { return size; }

		// Any elements.
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

		// Put a value. Returns 'true' if the key did not exist before.
		Bool CODECALL putRaw(const void *key, const void *value);

		// Contains value?
		Bool CODECALL hasRaw(const void *key);

		// Get a value. Throws if it does not exist.
		void *CODECALL getRaw(const void *key);

		// Get a value. Returns 'def' if it does not exist.
		void *CODECALL getRawDef(const void *key, const void *def);

		// Get a value, return a pointer to the value in the data structure. Return null if it did
		// not exist.
		void *CODECALL getUnsafeRaw(const void *key);

		// Get a value. Create using the constructor if it does not exist.
		template <class CreateCtor>
		void *atRaw(const void *key, CreateCtor fn) {
			nat hash = (*keyT.hashFn)(key);
			nat slot = findSlot(key, hash);

			if (slot == Info::free) {
				if (watch)
					// In case the object moved, we need to re-compute the hash.
					hash = newHash(key);
				nat w = Info::free;
				slot = insert(key, hash, w);
				fn(valPtr(slot), engine());
			}

			return valPtr(slot);
		}

		// Get a value. Create using the constructor if it does not exist. Assumes that 'v' is a value-type.
		typedef void (*SimpleCtor)(void *to);
		void *CODECALL atRawValue(const void *key, SimpleCtor fn);

		// Get a value. Create using the constructor if it does not exist. Assumes that 'V' is a pointer-type, we
		// will allocate memory for it before calling the constructor.
		void *CODECALL atRawClass(const void *key, Type *type, SimpleCtor fn);

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

		// Allocated memory. Split into three regions: info, key and value. Each of these are the
		// same number of elements in size.
		GcArray<Info> *info;
		GcArray<byte> *key;
		GcArray<byte> *val;

		// Watch if objects move (if needed).
		GcWatch *watch;

		// Our capacity.
		inline nat capacity() const { return info ? (nat)info->count : 0; }

		// Get locations for keys or values.
		inline void *keyPtr(nat id) { return key->v + id*keyT.size; }
		inline void *valPtr(nat id) { return val->v + id*valT.size; }
		inline const void *keyPtr(nat id) const { return key->v + id*keyT.size; }
		inline const void *valPtr(nat id) const { return val->v + id*valT.size; }

		inline void *keyPtr(GcArray<byte> *a, nat id) { return a->v + id*keyT.size; }
		inline void *valPtr(GcArray<byte> *a, nat id) { return a->v + id*valT.size; }

		// Allocate data for a specific capacity. Assumes 'info', 'key' and 'value' are null.
		void alloc(nat capacity);

		// Allocate data, assumes we are rehashing.
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
		// inserted into. 'watch' is a slot number that shall be updated if the contents of that
		// slot is ever moved.
		nat insert(const void *key, const void *val, nat hash, nat &watch);

		// Compute the insertion point for a new element. Does everything except copying the value
		// into the array. Do not skip copying the value, as the map will be left in an inconsistent
		// state. 'watch' is a slot number that shall be updated if the contents of that slot is
		// ever moved.
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
		 * rehash the table due to moved entries), we keep pointers to the data from the map in here
		 * so that iterators do not break when that happens.
		 */
		class Iter {
			STORM_VALUE;
		public:
			// Pointing to the end.
			Iter();

			// Pointing to the first element.
			Iter(MapBase *owner);

			// Pointing to an element.
			Iter(MapBase *owner, Nat pos);

			// Compare.
			bool CODECALL operator ==(const Iter &o) const;
			bool CODECALL operator !=(const Iter &o) const;

			// Advance.
			Iter &operator ++();
			Iter operator ++(int z);

			// Raw get functions.
			void *CODECALL rawKey() const;
			void *CODECALL rawVal() const;

			// Raw pre- and post increment.
			Iter &CODECALL preIncRaw();
			Iter CODECALL postIncRaw();

		private:
			// The three gc arrays from the map.
			// TODO: as the key and value arrays will eventually need to contain information about
			// which elements are free, we can elliminate 'info' eventually.
			GcArray<Info> *info;
			GcArray<byte> *key;
			GcArray<byte> *val;

			// Current position.
			Nat pos;

			// At end?
			bool atEnd() const;
		};

		// Raw begin and end.
		Iter CODECALL beginRaw();
		Iter CODECALL endRaw();

		// Find a value.
		Iter CODECALL findRaw(const void *key);

		// Friend.
		friend Iter;
	};

	// Let Storm know about the Map template.
	STORM_TEMPLATE(Map, createMap);

	/**
	 * C++ interface.
	 */
	template <class K, class V>
	class Map : public MapBase {
		STORM_SPECIAL;
	public:
		// Get the Storm type for this object.
		static Type *stormType(Engine &e) {
			return runtime::cppTemplate(e, MapId, 2, StormInfo<K>::id(), StormInfo<V>::id());
		}

		// Empty map.
		Map() : MapBase(StormInfo<K>::handle(engine()), StormInfo<V>::handle(engine())) {
			runtime::setVTable(this);
		}

		// Copy map.
		Map(Map<K, V> *o) : MapBase(o) {
			runtime::setVTable(this);
		}

		// Insert a value into the map, or update the existing one. Returns 'true' if the key did not exist before.
		Bool put(const K &k, const V &v) {
			return putRaw(&k, &v);
		}

		// Contains a key?
		Bool has(const K &k) {
			return hasRaw(&k);
		}

		// Get a value. Throws if not found.
		V &get(const K &k) {
			return *(V *)getRaw(&k);
		}

		// Get a value. Returns the default element if none found.
		V &get(const K &k, const V &def) {
			return *(V *)getRawDef(&k, &def);
		}

		// Get a value, create it if not alredy existing.
		// Note: Different from the Storm implementation!
		V &at(const K &k) {
			return *(V *)atRaw(&k, &CreateFn<V>::fn);
		}

		// Remove a value.
		Bool remove(const K &k) {
			return removeRaw(&k);
		}

		/**
		 * Iterator.
		 */
		class Iter : public MapBase::Iter {
		public:
			Iter() : MapBase::Iter() {}
			Iter(Map<K, V> *owner) : MapBase::Iter(owner) {}
			explicit Iter(MapBase::Iter i) : MapBase::Iter(i) {}

			std::pair<K, V&> operator *() const {
				return std::make_pair(*(K *)rawKey(), *(V *)rawVal());
			}

			// We can not provide the -> operator, so we adhere to the Storm convention of using k()
			// and v() instead.

			const K &k() const {
				return *(const K *)rawKey();
			}

			V &v() const {
				return *(V *)rawVal();
			}
		};

		// Find a value.
		Iter find(const K &k) {
			return Iter(findRaw(&k));
		}

		// Create the iterator.
		Iter begin() {
			return Iter(this);
		}

		Iter end() {
			return Iter();
		}
	};

	// Let Storm know about the RefMap template.
	STORM_TEMPLATE(RefMap, createRefMap);

	/**
	 * C++ interface.
	 */
	template <class K, class V>
	class RefMap : public MapBase {
		STORM_SPECIAL;
	public:
		// Get the Storm type for this object.
		static Type *stormType(Engine &e) {
			return runtime::cppTemplate(e, RefMapId, 2, StormInfo<K>::id(), StormInfo<V>::id());
		}

		// Empty map.
		RefMap() : MapBase(runtime::refObjHandle(engine()), StormInfo<V>::handle(engine())) {
			runtime::setVTable(this);
		}

		// Copy map.
		RefMap(RefMap<K, V> *o) : MapBase(o) {
			runtime::setVTable(this);
		}

		// Insert a value into the map, or update the existing one. Returns 'true' if the key did not exist before.
		Bool put(const K &k, const V &v) {
			return putRaw(&k, &v);
		}

		// Contains a key?
		Bool has(const K &k) {
			return hasRaw(&k);
		}

		// Get a value. Throws if not found.
		V &get(const K &k) {
			return *(V *)getRaw(&k);
		}

		// Get a value. Returns the default element if none found.
		V &get(const K &k, const V &def) {
			return *(V *)getRawDef(&k, &def);
		}

		// Get a value, create it if not alredy existing.
		V &at(const K &k) {
			return *(V *)atRaw(&k, &CreateFn<V>::fn);
		}

		// Remove a value.
		Bool remove(const K &k) {
			return removeRaw(&k);
		}

		/**
		 * Iterator.
		 */
		class Iter : public MapBase::Iter {
		public:
			Iter() : MapBase::Iter() {}
			Iter(RefMap<K, V> *owner) : MapBase::Iter(owner) {}
			explicit Iter(MapBase::Iter i) : MapBase::Iter(i) {}

			std::pair<K, V&> operator *() const {
				return std::make_pair(*(K *)rawKey(), *(V *)rawVal());
			}

			// We can not provide the -> operator, so we adhere to the Storm convention of using k()
			// and v() instead.

			const K &k() const {
				return *(const K *)rawKey();
			}

			V &v() const {
				return *(V *)rawVal();
			}
		};

		// Find a value.
		Iter find(const K &k) {
			return Iter(findRaw(&k));
		}

		// Create the iterator.
		Iter begin() {
			return Iter(this);
		}

		Iter end() {
			return Iter();
		}
	};
}
