#include "stdafx.h"
#include "VTableStorm.h"
#include "VTableCpp.h"

namespace storm {

	VTableStorm::VTableStorm(VTableCpp *update) : update(update) {}

	void VTableStorm::clear() {
		table = null;
		refs = null;
	}

	void VTableStorm::resize(Nat c) {
		size_t count = c;
		if (table != null && count <= table->count) {
			// Release possible GC retention.
			for (size_t i = min(table->filled, count); i < table->count; i++) {
				table->v[i] = null;
				refs->v[i] = null;
			}
			table->filled = count;
			refs->filled = count;

			// Note: we never shrink as of now.
			return;
		}

		// Grow! Note: growing 10 elements at a time as opposed to quadratic.
		size_t size = table ? table->count : 0;
		size = max(count, size + 10);

		GcArray<const void *> *n = runtime::allocArray<const void *>(engine(), &pointerArrayType, size);
		GcArray<Function *> *m = runtime::allocArray<Function *>(engine(), &pointerArrayType, size);
		n->filled = count;
		m->filled = count;
		if (table) {
			for (nat i = 0; i < table->filled; i++) {
				n->v[i] = table->v[i];
				m->v[i] = refs->v[i];
			}
		}

		table = n;
		refs = m;
		update->extra(table);
	}

	void VTableStorm::copyData(VTableStorm *src) {
		for (nat i = 0; i < min(src->count(), count()); i++)
			table->v[i] = src->table->v[i];
	}

	Nat VTableStorm::count() const {
		return table ? table->filled : 0;
	}

	Nat VTableStorm::dtorSlot() const {
		return 0;
	}

	Nat VTableStorm::freeSlot() const {
		return freeSlot(1);
	}

	Nat VTableStorm::freeSlot(Nat first) const {
		for (nat i = first; i < count(); i++) {
			if (refs->v[i] == null)
				return i;
		}

		return max(nat(1), count());
	}

	Nat VTableStorm::findSlot(Function *fn) const {
		for (nat i = 0; i < count(); i++) {
			if (refs->v[i] == fn)
				return i;
		}

		return vtable::invalid;
	}

	void VTableStorm::insert(Nat pos, Nat ins) {
		nat oldCount = count();
		resize(count() + ins);

		for (nat i = oldCount; i > pos; i--) {
			nat from = i - 1;
			nat to = from + ins;

			table->v[to] = table->v[from];
			refs->v[to] = refs->v[from];
			table->v[from] = null;
			refs->v[from] = null;
		}
	}

	void VTableStorm::set(Nat slot, Function *fn) {
		assert(slot < count(), L"Out of range: " + ::toS(slot));
		refs->v[slot] = fn;
		table->v[slot] = fn->directRef().address();
	}

	void VTableStorm::set(Nat slot, const void *addr) {
		assert(slot < count());
		table->v[slot] = addr;
	}

	Function *VTableStorm::get(Nat slot) const {
		if (slot < count())
			return refs->v[slot];

		return null;
	}

	void VTableStorm::clear(Nat slot) {
		assert(slot < count());
		refs->v[slot] = null;
		table->v[slot] = null;
	}

}
