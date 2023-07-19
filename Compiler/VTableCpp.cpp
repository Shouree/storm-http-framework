#include "stdafx.h"
#include "VTableCpp.h"
#include "Utils/Platform.h"
#include "Utils/Memory.h"
#include "Core/GcType.h"
#include "Exception.h"
#include "Function.h"
#include "Engine.h"

namespace storm {

	VTableCpp *VTableCpp::wrap(Engine &e, const void *vtable) {
		return new (e) VTableCpp(vtable, vtable::count(vtable), false);
	}

	VTableCpp *VTableCpp::wrap(Engine &e, const void *vtable, nat count) {
		return new (e) VTableCpp(vtable, count, false);
	}

	VTableCpp *VTableCpp::copy(Engine &e, const void *vtable) {
		return new (e) VTableCpp(vtable, vtable::count(vtable), true);
	}

	VTableCpp *VTableCpp::copy(Engine &e, const void *vtable, nat count) {
		return new (e) VTableCpp(vtable, count, true);
	}

	VTableCpp *VTableCpp::copy(Engine &e, const VTableCpp *src) {
		return new (e) VTableCpp(src->table(), src->count(), true);
	}

	VTableCpp::VTableCpp(const void *vtable, nat count, bool copy) {
		init(vtable, count, copy);
	}

	void VTableCpp::init(const void *vtable, nat count, bool copy) {
		refs = null;
		data = null;
		tabSize = count;
		raw = null;

		if (copy) {
			data = runtime::allocArray<const void *>(engine(), &pointerArrayType, count + vtable::extraOffset);
			data->filled = 0;

			const void ** src = (const void **)vtable - vtable::extraOffset;
			for (nat i = 1; i < count + vtable::extraOffset; i++) {
				data->v[i] = src[i];
			}
		} else {
			raw = (const void **)vtable;
		}
	}

	const void **VTableCpp::table() const {
		if (data)
			return &data->v[vtable::extraOffset];
		else
			return raw;
	}

	void VTableCpp::replace(const void *vtable) {
		replace(vtable, vtable::count(vtable));
	}

	void VTableCpp::replace(const VTableCpp *src) {
		replace(src->table(), src->count());
	}

	struct VTableSwitch {
		const void **from;
		const void **to;
	};

	static void vtableSwitch(RootObject *o, void *data) {
		VTableSwitch *s = (VTableSwitch *)data;

		if (vtable::from(o) == s->from)
			vtable::set(s->to, o);
	}

	void VTableCpp::replace(const void *vtable, nat count) {
		if (!data || count > this->count()) {
			// We need to replace the vtables on all live objects using the vtable.
			bool needWalk = used();
			VTableSwitch data;
			data.from = table();

			// Create the new vtable.
			init(vtable, count, true);

			data.to = table();

			// Don't walk the heap if we don't need to. That can be *very* expensive. In most cases
			// this happens right after we've created an object but before we have set the vtable to
			// an object, which means it is safe not to do the expensive heap walk.
			if (needWalk)
				engine().gc.walkObjects(&vtableSwitch, &data);
		} else {
			// We can copy, we know we can modify 'data'.
			const void *const* src = (const void *const*)vtable - vtable::extraOffset;
			for (nat i = 1; i < count + vtable::extraOffset; i++) {
				data->v[i] = src[i];
			}
		}
	}

	nat VTableCpp::count() const {
		return tabSize;
	}

	nat VTableCpp::findSlot(const void *fn) const {
		return vtable::find(table(), fn, count());
	}

	const void *VTableCpp::extra() const {
		if (data)
			return data->v[0];
		else
			return null;
	}

	void VTableCpp::extra(const void *to) {
		if (data)
			data->v[0] = to;
	}

	void VTableCpp::set(nat id, const void *to) {
		if (data)
			data->v[id + vtable::extraOffset] = to;
	}

	void VTableCpp::set(nat id, Function *fn) {
		assert(id < count());

		if (!refs)
			refs = runtime::allocArray<Function *>(engine(), &pointerArrayType, count());

		refs->v[id] = fn;
		set(id, fn->directRef().address());
	}

	Function *VTableCpp::get(nat id) const {
		if (refs)
			if (id < count())
				return refs->v[id];

		return null;
	}

	void VTableCpp::clear(nat id) {
		assert(id < count());

		if (refs) {
			refs->v[id] = null;
		}
		if (data)
			data->v[id + vtable::extraOffset] = null;
	}

	const void *VTableCpp::address() const {
		if (data)
			return data;
		else
			// Tag the pointer.
			return (const byte *)raw + 1;
	}

	nat VTableCpp::size() const {
		return Nat(count() * sizeof(const void *) + vtableAllocOffset());
	}

	bool VTableCpp::used() const {
		if (data)
			return data->filled != 0;
		else
			return true;
	}

	void VTableCpp::insert(RootObject *obj) {
		if (data)
			data->filled = 1;
		vtable::set(table(), obj);
	}

	void VTableCpp::insert(code::Listing *to, code::Var obj, code::Ref table) {
		using namespace code;
		Label lbl = to->label();

		// See if the pointer in 'table' is tagged with a 1 in the lower bit. If so, remove it and
		// skip the mark-phase since we're dealing with a raw vtable.
		*to << mov(ptrA, table);
		*to << mov(ptrC, ptrConst(1));
		*to << bnot(ptrC);
		*to << band(ptrC, ptrA);
		*to << cmp(ptrC, ptrA);
		*to << jmp(lbl, ifNotEqual);

		// Mark as used by setting 'filled' to 1.
		*to << mov(ptrRel(ptrA, Offset::sPtr), ptrConst(1));
		// The pointer is offset a bit.
		*to << add(ptrA, to->engine().ref(builtin::VTableAllocOffset));

		*to << lbl;
		*to << mov(ptrC, obj);
		*to << mov(ptrRel(ptrC, Offset()), ptrA);
	}
}
