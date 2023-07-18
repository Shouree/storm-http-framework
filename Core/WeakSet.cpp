#include "stdafx.h"
#include "WeakSet.h"
#include "GcType.h"
#include "GcWatch.h"
#include "Hash.h"
#include "StrBuf.h"
#include "Utils/Bitwise.h"

namespace storm {

	const nat WeakSetBase::minCapacity = 4;

	const GcType WeakSetBase::infoType = {
		GcType::tArray,
		null,
		null,
		sizeof(Info),
		0,
		{},
	};

	WeakSetBase::WeakSetBase() : watch(runtime::createWatch(engine())) {}

	WeakSetBase::WeakSetBase(const WeakSetBase &other) {
		size = other.size;
		lastFree = other.lastFree;
		info = copyArray(other.info);
		data = copyArray(other.data);
		watch = other.watch->clone();
	}

	void WeakSetBase::deepCopy(CloneEnv *env) {
		// Nothing needs to be done. We only store TObjects, which do not need to be deepCopied.
	}

	void WeakSetBase::clear() {
		info = null;
		data = null;
		size = 0;
		lastFree = 0;
		watch->clear();
	}

	void WeakSetBase::shrink() {
		nat size = 0;
		for (nat i = 0; i < capacity(); i++)
			if (data->v[i])
				size++;

		if (size == 0) {
			clear();
			return;
		}

		nat to = max(minCapacity, nextPowerOfTwo(size));
		rehash(to);
	}

	void WeakSetBase::toS(StrBuf *to) const {
		*to << S("{");
		bool first = true;

		for (nat i = 0; i < capacity(); i++) {
			if (info->v[i].status == Info::free)
				continue;

			if (!first)
				*to << S(", ");
			first = false;

			*to << data->v[i];
		}

		*to << S("}");
	}

	Bool WeakSetBase::putRaw(TObject *key) {
		clean();

		nat hash = ptrHash(key);
		nat old = findSlot(key, hash);
		if (old == Info::free) {
			if (watch) {
				// In case the object moved, we need to re-compute the hash.
				watch->add(key);
				hash = ptrHash(key);
			}
			nat w = Info::free;
			insert(key, hash, w);
			return true;
		} else {
			data->v[old] = key;
			return false;
		}
	}

	Bool WeakSetBase::hasRaw(TObject *key) {
		clean();

		nat hash = ptrHash(key);
		return findSlot(key, hash) != Info::free;
	}

	Bool WeakSetBase::removeRaw(TObject *key) {
		// Must be first: otherwise, we might shrink to zero.
		clean();

		// Will break 'primarySlot' otherwise.
		if (capacity() == 0)
			return false;

		if (remove(key)) {
			return true;
		} else if (watch != null && watch->moved(key)) {
			return rehashRemove(capacity(), key);
		} else {
			return false;
		}
	}

	bool WeakSetBase::remove(TObject *key) {
		nat hash = ptrHash(key);
		nat slot = primarySlot(hash);

		// Not in the map?
		if (info->v[slot].status == Info::free)
			return false;

		nat prev = Info::free;
		do {
			if (info->v[slot].hash == hash && key == data->v[slot]) {
				// Is the node we're about to delete inside a chain?
				if (prev != Info::free) {
					// Unlink us from the chain.
					info->v[prev].status = info->v[slot].status;
				}

				nat next = info->v[slot].status;

				// Destroy the node.
				info->v[slot].status = Info::free;
				data->v[slot] = null;

				if (prev == Info::free && next != Info::end) {
					// The removed node was in the primary slot, and we need to move the next one into our slot.
					data->v[slot] = data->v[next];
					info->v[slot] = info->v[next];
					info->v[next].status = Info::free;
					data->v[next] = null;
				}

				if (watch)
					watch->remove(key);
				size--;
				return true;
			}

			prev = slot;
			slot = info->v[slot].status;
		} while (slot != Info::end);

		return false;
	}

	Nat WeakSetBase::countCollisions() const {
		Nat c = 0;
		for (nat i = 0; i < capacity(); i++) {
			if (info->v[i].status != Info::free && i == primarySlot(info->v[i].hash)) {
				for (nat at = i; info->v[at].status != Info::end; at++)
					c++;
			}
		}
		return c;
	}

	Nat WeakSetBase::countMaxChain() const {
		Nat c = 0;
		for (nat i = 0; i < capacity(); i++) {
			if (info->v[i].status != Info::free && i == primarySlot(info->v[i].hash)) {
				Nat len = 1;
				for (nat at = i; info->v[at].status != Info::end; at++)
					len++;
				c = max(c, len);
			}
		}
		return c;
	}

	void WeakSetBase::alloc(nat cap) {
		assert(info == null);
		assert(data == null);
		assert(isPowerOfTwo(cap));

		size = 0;
		lastFree = 0;
		info = runtime::allocArray<Info>(engine(), &infoType, cap);
		data = runtime::allocWeakArray<TObject>(engine(), cap);

		for (nat i = 0; i < cap; i++)
			info->v[i].status = Info::free;
	}

	void WeakSetBase::allocRehash(nat cap) {
		assert(info == null);
		assert(data == null);
		assert(isPowerOfTwo(cap));

		size = 0;
		lastFree = 0;
		info = runtime::allocArrayRehash<Info>(engine(), &infoType, cap);
		data = runtime::allocWeakArrayRehash<TObject>(engine(), cap);

		for (nat i = 0; i < cap; i++)
			info->v[i].status = Info::free;
	}

	void WeakSetBase::grow() {
		nat c = capacity();
		if (c == 0) {
			// Initial table size.
			alloc(minCapacity);
		} else if (c == size) {
			// Keep to a multiple of 2.
			rehash(c * 2);
		}
	}

	void WeakSetBase::rehash(nat cap) {
		Nat oldSize = size;

		GcArray<Info> *oldInfo = info; info = null;
		GcWeakArray<TObject> *oldData = data; data = null;
		GcWatch *oldWatch = watch; watch = runtime::createWatch(engine());

		alloc(cap);

		// Anything to do?
		if (oldInfo == null)
			return;

		try {
			nat w = Info::free;

			// Insert all elements once again.
			for (nat i = 0; i < oldInfo->count; i++) {
				TObject *k = oldData->v[i];
				if (oldInfo->v[i].status == Info::free || k == null)
					continue;

				watch->add(k);
				nat hash = ptrHash(k);
				insert(k, hash, w);
			}

			// The Gc will destroy the old arrays and all elements in there later on.
		} catch (...) {
			clear();

			// Restore old state.
			swap(oldSize, size);
			swap(oldInfo, info);
			swap(oldData, data);
			swap(oldWatch, watch);
			throw;
		}
	}

	nat WeakSetBase::rehashFind(nat cap, TObject *find) {
		Nat oldSize = size;

		GcArray<Info> *oldInfo = info; info = null;
		GcWeakArray<TObject> *oldData = data; data = null;
		GcWatch *oldWatch = watch; watch = runtime::createWatch(engine());

		allocRehash(cap);

		// Anything to do?
		if (oldInfo == null)
			return Info::free;

		try {
			nat found = Info::free;

			// Insert all elements once again.
			for (nat i = 0; i < oldInfo->count; i++) {
				TObject *k = oldData->v[i];
				// Skip free slots and splatted slots.
				if (oldInfo->v[i].status == Info::free || k == null)
					continue;

				// We need to re-hash here, as some objects have moved.
				watch->add(k);
				nat hash = ptrHash(k);
				nat into = insert(k, hash, found);

				// Is this the key we're looking for?
				if (find == k)
					found = into;
			}

			// The Gc will destroy the old arrays and all elements in there later on.
			return found;
		} catch (...) {
			clear();

			// Restore old state.
			swap(oldSize, size);
			swap(oldInfo, info);
			swap(oldData, data);
			swap(oldWatch, watch);
			throw;
		}
	}

	bool WeakSetBase::rehashRemove(nat cap, TObject *remove) {
		Nat oldSize = size;

		GcArray<Info> *oldInfo = info; info = null;
		GcWeakArray<TObject> *oldData = data; data = null;
		GcWatch *oldWatch = watch; watch = runtime::createWatch(engine());

		allocRehash(cap);

		// Anything to do?
		if (oldInfo == null)
			return false;

		try {
			bool found = false;
			nat w = Info::free;

			// Insert all elements once again.
			for (nat i = 0; i < oldInfo->count; i++) {
				TObject *k = data->v[i];
				// Skip free slots and splatted slots.
				if (oldInfo->v[i].status == Info::free || k == null)
					continue;


				// Is this the key we're looking for?
				if (k == remove) {
					found = true;
					continue;
				}

				// We need to re-hash here, as some objects have moved.
				watch->add(k);
				nat hash = ptrHash(k);
				insert(k, hash, w);
			}

			// The Gc will destroy the old arrays and all elements in there later on.
			return found;
		} catch (...) {
			clear();

			// Restore old state.
			swap(oldSize, size);
			swap(oldInfo, info);
			swap(oldData, data);
			swap(oldWatch, watch);
			throw;
		}
	}

	nat WeakSetBase::insert(TObject *key, nat hash, nat &watch) {
		grow();

		Info insert = { Info::end, hash };
		nat into = primarySlot(hash);

		if (info->v[into].status != Info::free) {
			// Check if the contained element is in its primary position.
			nat from = primarySlot(info->v[into].hash);
			if (from == into) {
				// It is in its primary position. Attach ourselves to the chain.
				nat to = freeSlot();
				insert.status = info->v[into].status;
				info->v[into].status = to;
				into = to;
			} else {
				// It is not. Move it somewhere else.

				// Walk the list from the original position and find the node before the one we're about to move...
				while (info->v[from].status != into)
					from = info->v[from].status;

				// Redo linking.
				nat to = freeSlot();
				info->v[from].status = to;

				// Move the node itself.
				info->v[to] = info->v[into];
				data->v[to] = data->v[into];
				data->v[into] = null;
				info->v[into].status = Info::free;

				// Update watched slot.
				if (watch == into)
					watch = to;
			}
		}

		assert(info->v[into].status == Info::free, L"Internal error. Trying to overwrite a slot!");
		info->v[into] = insert;
		data->v[into] = key;
		size++;

		return into;
	}

	nat WeakSetBase::findSlot(TObject *key, nat hash) {
		// Otherwise, primarySlot won't work.
		if (capacity() == 0)
			return Info::free;

		nat r = findSlotI(key, hash);
		if (r == Info::free && watch != null) {
			if (watch->moved(key))
				// The object has moved. We need to rebuild the hash map.
				r = rehashFind(capacity(), key);
		}

		return r;
	}

	nat WeakSetBase::findSlotI(TObject *key, nat hash) {
		// We assume 'capacity() > 0', as checked by 'findSlot'.
		nat slot = primarySlot(hash);
		if (info->v[slot].status == Info::free)
			return Info::free;

		do {
			if (info->v[slot].hash == hash && key == data->v[slot])
				return slot;

			slot = info->v[slot].status;
		} while (slot != Info::end);

		return Info::free;
	}

	nat WeakSetBase::primarySlot(nat hash) const {
		// We know that 'capacity' is a power of two, therefore the following is equivalent to:
		// return hash % capacity;
		return hash & (capacity() - 1);
	}

	nat WeakSetBase::freeSlot() {
		while (info->v[lastFree].status != Info::free)
			// We know that 'capacity' is a power of two. Therefore, the following is equivalent to:
			// if (++lastFree >= capacity) lastFree = 0;
			lastFree = (lastFree + 1) & (capacity() - 1);

		return lastFree;
	}

	void WeakSetBase::clean() {
		if (data) {
			// Clean when more than 1/3 of references have been splatted.
			if (data->splatted() * 3 >= data->count()) {
				shrink();
			}
		}
	}

	GcArray<WeakSetBase::Info> *WeakSetBase::copyArray(const GcArray<Info> *src) {
		if (!src)
			return null;

		GcArray<Info> *dest = runtime::allocArray<Info>(engine(), &infoType, src->count);
		memcpy(dest->v, src->v, src->count*sizeof(Info));
		return dest;
	}

	GcWeakArray<TObject> *WeakSetBase::copyArray(const GcWeakArray<TObject> *src) {
		if (!src)
			return null;

		GcWeakArray<TObject> *dest = runtime::allocWeakArray<TObject>(engine(), src->count());
		memcpy(dest->v, src->v, src->count()*sizeof(TObject *));
		return dest;
	}


	WeakSetBase::Iter::Iter() : data(null), pos(0) {}

	WeakSetBase::Iter::Iter(WeakSetBase *owner) : data(owner->data), pos(0) {}

	TObject *WeakSetBase::Iter::nextRaw() {
		if (!data)
			return null;

		while (data != null && pos < data->count()) {
			nat last = pos++;

			// Make sure the object is not pulled from under our feet.
			TObject *obj = atomicRead(data->v[last]);

			// See if the object exists, and if it has been finalized but not yet
			// collected. Otherwise we might return a reference to a finalized object, which will
			// likely cause crashes.
			if (obj && runtime::liveObject(obj))
				return obj;
		}

		// At the end.
		return null;
	}

	WeakSetBase::Iter WeakSetBase::iterRaw() {
		return Iter(this);
	}

}
