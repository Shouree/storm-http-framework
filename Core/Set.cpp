#include "stdafx.h"
#include "Set.h"
#include "Hash.h"
#include "StrBuf.h"
#include "GcType.h"
#include "GcWatch.h"
#include "Utils/Bitwise.h"
#include "Exception.h"
#include <iomanip>

namespace storm {

	const nat SetBase::minCapacity = 4;

	const GcType SetBase::infoType = {
		GcType::tArray,
		null,
		null,
		sizeof(Info),
		0,
		{},
	};

	SetBase::SetBase(const Handle &k) : keyT(k), watch(null) {
		checkHashHandle(k);
		if (k.locationHash)
			watch = runtime::createWatch(engine());
	}

	SetBase::SetBase(const SetBase &o) : keyT(o.keyT), watch(null) {
		size = o.size;
		lastFree = o.lastFree;
		info = copyArray(o.info);
		key = copyArray(o.key, info, keyT);

		if (o.watch)
			watch = o.watch->clone();
	}

	void SetBase::deepCopy(CloneEnv *env) {
		if (keyT.deepCopyFn) {
			for (size_t i = 0; i < capacity(); i++) {
				if (info->v[i].status != Info::free) {
					(*keyT.deepCopyFn)(keyPtr(Nat(i)), env);
				}
			}
		}
	}

	void SetBase::clear() {
		size = 0;
		lastFree = 0;
		info = null;
		key = null;
		if (watch)
			watch->clear();
	}

	void SetBase::shrink() {
		if (size == 0) {
			clear();
			return;
		}

		nat to = max(minCapacity, nextPowerOfTwo(size));
		rehash(to);
	}

	void SetBase::toS(StrBuf *to) const {
		*to << S("{");
		bool first = true;

		for (nat i = 0; i < capacity(); i++) {
			if (info->v[i].status == Info::free)
				continue;

			if (!first)
				*to << S(", ");
			first = false;

			(*keyT.toSFn)(keyPtr(i), to);
		}

		*to << S("}");
	}

	nat SetBase::newHash(const void *key) {
		if (watch) {
			// Place the pointer on the stack to prevent it from moving around while we're
			// registering we're depending on it.
			const void *kPtr = *(const void **)key;
			watch->add(kPtr);
			return (*keyT.hashFn)(key);
		} else {
			return (*keyT.hashFn)(key);
		}
	}

	bool SetBase::changedHash(const void *key) {
		if (watch) {
			const void *kPtr = *(const void **)key;
			return watch->moved(kPtr);
		} else {
			return false;
		}
	}

	Bool SetBase::putRaw(const void *key) {
		nat hash = (*keyT.hashFn)(key);
		nat old = findSlot(key, hash);
		if (old == Info::free) {
			if (watch)
				// In case the object moved, we need to re-compute the hash.
				hash = newHash(key);
			nat w = Info::free;
			insert(key, hash, w);
			return true;
		} else {
			keyT.safeDestroy(keyPtr(old));
			keyT.safeCopy(keyPtr(old), key);
			return false;
		}
	}

	void SetBase::putSetRaw(SetBase *from) {
		// Do not update ourselves if someone tries to add the set to itself. This will break the iteration.
		if (this == from)
			return;

		for (Iter i = from->beginRaw(), e = from->endRaw(); i != e; ++i)
			putRaw(i.rawVal());
	}

	Bool SetBase::hasRaw(const void *key) {
		nat hash = (*keyT.hashFn)(key);
		return findSlot(key, hash) != Info::free;
	}

	void *SetBase::getRaw(const void *key) {
		void *result = getUnsafeRaw(key);
		if (!result) {
			StrBuf *buf = new (this) StrBuf();
			*buf << L"The key ";
			(*keyT.toSFn)(key, buf);
			*buf << L" is not in the map.";
			throw new (this) SetError(buf->toS());
		}
		return result;
	}

	void *SetBase::getUnsafeRaw(const void *key) {
		nat hash = (*keyT.hashFn)(key);
		nat slot = findSlot(key, hash);
		if (slot == Info::free)
			return null;
		else
			return keyPtr(slot);
	}

	void *SetBase::atRaw(const void *key) {
		nat hash = (*keyT.hashFn)(key);
		nat slot = findSlot(key, hash);

		if (slot == Info::free) {
			if (watch)
				// In case the object moved, we need to re-compute the hash.
				hash = newHash(key);
			nat w = Info::free;
			slot = insert(key, hash, w);
		}

		return keyPtr(slot);
	}

	Bool SetBase::removeRaw(const void *key) {
		// Will break 'primarySlot' otherwise.
		if (capacity() == 0)
			return false;

		if (remove(key)) {
			return true;
		} else if (changedHash(key)) {
			return rehashRemove(capacity(), key);
		} else {
			return false;
		}
	}

	bool SetBase::remove(const void *key) {
		nat hash = (*keyT.hashFn)(key);
		nat slot = primarySlot(hash);

		// Not in the map?
		if (info->v[slot].status == Info::free)
			return false;

		nat prev = Info::free;
		do {
			if (info->v[slot].hash == hash && keyT.equal(key, keyPtr(slot))) {
				// Is the node we're about to delete inside a chain?
				if (prev != Info::free) {
					// Unlink us from the chain.
					info->v[prev].status = info->v[slot].status;
				}

				nat next = info->v[slot].status;

				// Destroy the node.
				info->v[slot].status = Info::free;
				keyT.safeDestroy(keyPtr(slot));

				if (prev == Info::free && next != Info::end) {
					// The removed node was in the primary slot, and we need to move the next one into our slot.
					keyT.safeCopy(keyPtr(slot), keyPtr(next));
					info->v[slot] = info->v[next];
					info->v[next].status = Info::free;
					keyT.safeDestroy(keyPtr(next));
				}

				size--;
				if (watch)
					watch->remove(key);
				return true;
			}

			prev = slot;
			slot = info->v[slot].status;
		} while (slot != Info::end);

		return false;
	}

	Nat SetBase::countCollisions() const {
		Nat c = 0;
		for (nat i = 0; i < capacity(); i++) {
			if (info->v[i].status != Info::free && i == primarySlot(info->v[i].hash)) {
				for (nat at = i; info->v[at].status != Info::end; at++)
					c++;
			}
		}
		return c;
	}

	Nat SetBase::countMaxChain() const {
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

	void SetBase::dbg_print() {
		std::wcout << L"Map contents:" << endl;
		for (nat i = 0; i < capacity(); i++) {
			std::wcout << std::setw(2) << i << L": ";
			if (info->v[i].status == Info::free) {
				std::wcout << L"free";
			} else if (info->v[i].status == Info::end) {
				std::wcout << toHex(info->v[i].hash) << L" end";
			} else {
				std::wcout << toHex(info->v[i].hash) << L" -> " << info->v[i].status;
			}

			if (info->v[i].status != Info::free) {
				std::wcout << "   ";
				StrBuf *b = new (this) StrBuf();
				(*keyT.toSFn)(keyPtr(i), b);
				std::wcout << b;
			}
			std::wcout << endl;
		}
	}

	void SetBase::alloc(nat cap) {
		assert(info == null);
		assert(key == null);
		assert(isPowerOfTwo(cap));

		size = 0;
		lastFree = 0;
		info = runtime::allocArray<Info>(engine(), &infoType, cap);
		key = runtime::allocArray<byte>(engine(), keyT.gcArrayType, cap);

		for (nat i = 0; i < cap; i++)
			info->v[i].status = Info::free;
	}

	void SetBase::allocRehash(nat cap) {
		assert(info == null);
		assert(key == null);
		assert(isPowerOfTwo(cap));

		size = 0;
		lastFree = 0;
		info = runtime::allocArrayRehash<Info>(engine(), &infoType, cap);
		key = runtime::allocArrayRehash<byte>(engine(), keyT.gcArrayType, cap);

		for (nat i = 0; i < cap; i++)
			info->v[i].status = Info::free;
	}

	void SetBase::grow() {
		nat c = capacity();
		if (c == 0) {
			// Initial table size.
			alloc(minCapacity);
		} else if (c == size) {
			// Keep to a multiple of 2.
			rehash(c * 2);
		}
	}

	void SetBase::rehash(nat cap) {
		nat oldSize = size;
		GcArray<Info> *oldInfo = info; info = null;
		GcArray<byte> *oldKey = key; key = null;

		alloc(cap);

		// Anything to do?
		if (oldInfo == null)
			return;

		try {
			nat w = Info::free;

			// Insert all elements once again.
			for (nat i = 0; i < oldInfo->count; i++) {
				if (oldInfo->v[i].status == Info::free)
					continue;

				insert(keyPtr(oldKey, i), oldInfo->v[i].hash, w);
			}

			// The Gc will destroy the old arrays and all elements in there later on.
		} catch (...) {
			clear();

			// Restore old state.
			swap(oldSize, size);
			swap(oldInfo, info);
			swap(oldKey, key);
			throw;
		}
	}

	nat SetBase::rehashFind(nat cap, const void *find) {
		nat oldSize = size;
		GcArray<Info> *oldInfo = info; info = null;
		GcArray<byte> *oldKey = key; key = null;
		GcWatch *oldWatch = watch; watch = runtime::createWatch(engine());

		allocRehash(cap);

		// Anything to do?
		if (oldInfo == null)
			return Info::free;

		try {
			nat found = Info::free;

			// Insert all elements once again.
			for (nat i = 0; i < oldInfo->count; i++) {
				if (oldInfo->v[i].status == Info::free)
					continue;

				// We need to re-hash here, as some objects have moved.
				const void *k = keyPtr(oldKey, i);
				nat hash = newHash(k);
				nat into = insert(k, hash, found);

				// Is this the key we're looking for?
				if (keyT.equal(k, find))
					found = into;
			}

			// The Gc will destroy the old arrays and all elements in there later on.
			return found;
		} catch (...) {
			clear();

			// Restore old state.
			swap(oldSize, size);
			swap(oldInfo, info);
			swap(oldKey, key);
			swap(oldWatch, watch);
			throw;
		}
	}

	bool SetBase::rehashRemove(nat cap, const void *remove) {
		nat oldSize = size;
		GcArray<Info> *oldInfo = info; info = null;
		GcArray<byte> *oldKey = key; key = null;
		GcWatch *oldWatch = watch; watch = runtime::createWatch(engine());

		allocRehash(cap);

		// Anything to do?
		if (oldInfo == null)
			return false;

		watch->clear();

		try {
			bool found = false;
			nat w = Info::free;

			// Insert all elements once again.
			for (nat i = 0; i < oldInfo->count; i++) {
				if (oldInfo->v[i].status == Info::free)
					continue;

				const void *k = keyPtr(oldKey, i);

				// Is this the key we're looking for?
				if (keyT.equal(k, remove)) {
					found = true;
					continue;
				}

				// We need to re-hash here, as some objects have moved.
				nat hash = newHash(k);
				insert(k, hash, w);
			}

			// The Gc will destroy the old arrays and all elements in there later on.
			return found;
		} catch (...) {
			clear();

			// Restore old state.
			swap(oldSize, size);
			swap(oldInfo, info);
			swap(oldKey, key);
			swap(oldWatch, watch);
			throw;
		}
	}

	nat SetBase::insert(const void *key, nat hash, nat &watch) {
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
				keyT.safeCopy(keyPtr(to), keyPtr(into));
				keyT.safeDestroy(keyPtr(into));
				info->v[into].status = Info::free;

				// Update watched slot.
				if (watch == into)
					watch = to;
			}
		}

		assert(info->v[into].status == Info::free, L"Internal error. Trying to overwrite a slot!");
		info->v[into] = insert;
		keyT.safeCopy(keyPtr(into), key);
		size++;

		return into;
	}

	nat SetBase::findSlot(const void *key, nat hash) {
		// Otherwise, primarySlot won't work.
		if (capacity() == 0)
			return Info::free;

		nat r = findSlotI(key, hash);
		if (r == Info::free && changedHash(key)) {
			// The object has moved. We need to rebuild the hash map.
			r = rehashFind(capacity(), key);
		}

		return r;
	}

	nat SetBase::findSlotI(const void *key, nat hash) {
		// We assume 'capacity() > 0', as checked by 'findSlot'.
		nat slot = primarySlot(hash);
		if (info->v[slot].status == Info::free)
			return Info::free;

		do {
			if (info->v[slot].hash == hash && keyT.equal(key, keyPtr(slot)))
				return slot;

			slot = info->v[slot].status;
		} while (slot != Info::end);

		return Info::free;
	}

	nat SetBase::primarySlot(nat hash) const {
		// We know that 'capacity' is a power of two, therefore the following is equivalent to:
		// return hash % capacity;
		return hash & (capacity() - 1);
	}

	nat SetBase::freeSlot() {
		while (info->v[lastFree].status != Info::free)
			// We know that 'capacity' is a power of two. Therefore, the following is equivalent to:
			// if (++lastFree >= capacity) lastFree = 0;
			lastFree = (lastFree + 1) & (capacity() - 1);

		return lastFree;
	}

	GcArray<SetBase::Info> *SetBase::copyArray(const GcArray<Info> *src) {
		if (!src)
			return null;

		GcArray<Info> *dest = runtime::allocArray<Info>(engine(), &infoType, src->count);
		memcpy(dest->v, src->v, src->count*sizeof(Info));
		return dest;
	}

	GcArray<byte> *SetBase::copyArray(const GcArray<byte> *src, const GcArray<Info> *info, const Handle &type) {
		if (!src)
			return null;

		GcArray<byte> *dest = runtime::allocArray<byte>(engine(), type.gcArrayType, src->count);

		if (type.copyFn) {
			const byte *srcNow = src->v;
			byte *dstNow = dest->v;
			for (nat i = 0; i < src->count; i++) {
				// Copy only if there are valid objects there.
				if (info->v[i].status != Info::free)
					(*type.copyFn)(dstNow, srcNow);
				srcNow += type.size;
				dstNow += type.size;
			}
		} else {
			memcpy(dest->v, src->v, type.size*src->count);
		}

		return dest;
	}

	SetBase::Iter::Iter() : info(null), key(null), pos(0) {}

	SetBase::Iter::Iter(SetBase *owner) : info(owner->info), key(owner->key), pos(0) {
		// Find the first occupied position. This may place us at the end.
		while (!atEnd() && info->v[pos].status == Info::free)
			pos++;
	}

	bool SetBase::Iter::operator ==(const Iter &o) const {
		if (atEnd() && o.atEnd())
			return true;
		else
			return info == o.info && pos == o.pos;
	}

	bool SetBase::Iter::operator !=(const Iter &o) const {
		return !(*this == o);
	}

	SetBase::Iter &SetBase::Iter::operator ++() {
		if (!atEnd())
			pos++;

		// Find the next occupied position.
		while (!atEnd() && info->v[pos].status == Info::free)
			pos++;

		return *this;
	}

	SetBase::Iter SetBase::Iter::operator ++(int) {
		Iter t(*this);
		++*this;
		return t;
	}

	void *SetBase::Iter::rawVal() const {
		size_t s = runtime::gcTypeOf(key)->stride;
		return key->v + pos*s;
	}

	SetBase::Iter &SetBase::Iter::preIncRaw() {
		return operator ++();
	}

	SetBase::Iter SetBase::Iter::postIncRaw() {
		return operator ++(0);
	}

	bool SetBase::Iter::atEnd() const {
		if (info)
			return pos == info->count;
		else
			return true;
	}

	SetBase::Iter SetBase::beginRaw() {
		return Iter(this);
	}

	SetBase::Iter SetBase::endRaw() {
		return Iter();
	}

}
