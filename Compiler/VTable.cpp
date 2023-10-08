#include "stdafx.h"
#include "VTable.h"
#include "Type.h"
#include "Engine.h"
#include "Core/Str.h"
#include <iomanip>

namespace storm {

	VTable::VTable(Type *owner) : owner(owner) {}

	void VTable::createCpp(const void *cppVTable) {
		assert(cpp == null, L"Can not re-use a VTable for a new C++ root object.");
		stormFirst = 1;

		cpp = VTableCpp::wrap(engine(), cppVTable);

		if (engine().has(bootTemplates))
			lateInit();
	}

	void VTable::createStorm(VTable *parent) {
		// Disable vtable lookup for all functions already in here.
		if (updaters) {
			for (UpdateMap::Iter i = updaters->begin(), e = updaters->end(); i != e; ++i) {
				i.k()->setLookup(null);
			}
			updaters = new (this) UpdateMap();
		}

		if (cpp)
			cpp->replace(parent->cpp);
		else
			cpp = VTableCpp::copy(engine(), parent->cpp);
		if (source)
			source->set(cpp);

		// Create a new Storm VTable.
		storm = new (this) VTableStorm(cpp);
		stormFirst = parent->storm
			? parent->storm->count()
			: 1; // always place stuff after the destructor slot.
		storm->resize(stormFirst);
		if (parent->storm)
			storm->copyData(parent->storm);

		if (engine().has(bootTemplates))
			lateInit();
	}

	void VTable::lateInit() {
		if (!updaters)
			updaters = new (this) UpdateMap();

		if (!source) {
			source = new (this) NamedSource(owner, Char('v'));
			source->set(cpp);
		}
	}

	void VTable::insert(RootObject *obj) {
		cpp->insert(obj);
	}

	void VTable::insert(code::Listing *to, code::Var obj) {
		cpp->insert(to, obj, code::Ref(source));
	}

	Array<Function *> *VTable::allSlots() {
		Array<Function *> *cppFns = new (this) Array<Function *>(cpp->count(), null);
		Array<Function *> *stormFns = null;
		if (storm)
			stormFns = new (this) Array<Function *>(storm->count(), null);

		allSlots(cppFns, stormFns);

		Array<Function *> *result = new (this) Array<Function *>();
		result->reserve(cppFns->count() + (stormFns ? stormFns->count() : 0));
		for (Nat i = 0; i < cppFns->count(); i++)
			if (Function *f = cppFns->at(i))
				result->push(f);

		if (storm)
			for (Nat i = 0; i < stormFns->count(); i++)
				if (Function *f = stormFns->at(i))
					result->push(f);

		return result;
	}

	void VTable::allSlots(Array<Function *> *cppFns, Array<Function *> *stormFns) {
		for (Nat i = 0; i < min(cppFns->count(), cpp->count()); i++) {
			if (!cppFns->at(i))
				cppFns->at(i) = cpp->get(i);
		}

		if (storm) {
			for (Nat i = 0; i < min(stormFns->count(), storm->count()); i++) {
				if (!stormFns->at(i))
					stormFns->at(i) = storm->get(i);
			}
		}

		if (Type *s = owner->super())
			rawVTable(s)->allSlots(cppFns, stormFns);
	}

	void VTable::dbg_dump() const {
		PLN(L"vtable @" << cpp->address() << L":");

		PLN(L"C++:");
		const void **ptr = (const void **)cpp->address();
		ptr += 4; // Skip metadata.
		for (nat i = 0; i < cpp->count(); i++) {
			PNN(std::setw(4) << i << L": " << ptr[i]);
			if (Function *f = cpp->get(i))
				PLN(L" (" << f << L")");
			else
				PLN(L"");
		}

		if (storm) {
			const void **ptr = (const void **)cpp->extra();
			PLN(L"Storm: " << ptr);
			for (nat i = 0; i < storm->count(); i++) {
				PNN(std::setw(4) << i << L": " << ptr[i+2]);
				if (Function *f = storm->get(i))
					PLN(L" (" << f << L")");
				else
					PLN(L"");
			}
		}
	}

	void VTable::insert(Function *fn) {
		// Do we already know this function?
		if (updaters->has(fn))
			return;

		VTableSlot slot = findSlot(fn, true);

		if (!slot.valid()) {
			// We need to allocate a slot for this function.
			slot = allocSlot();
		}

		// Insert into the vtable.
		set(slot, fn);
		updaters->put(fn, new (this) VTableUpdater(this, slot, fn->directRef(), source->content()));

		// See if there are any child functions which we potentially need to update.
		if (updateChildren(fn, slot))
			useLookup(fn, slot);
	}

	void VTable::removeChild(Type *child) {
		// For all slots: find a match to that function in a pre-existing child class.

		nat stormCount = storm ? storm->count() : 1;
		GcBitset *cppFound = allocBitset(engine(), cpp->count());
		GcBitset *stormFound = allocBitset(engine(), stormCount);

		disableLookup(cppFound, stormFound);
	}

	void VTable::disableLookup(GcBitset *cppFound, GcBitset *stormFound) {
		// We need to check this, since in early boot we may not yet have created the map.
		if (updaters) {
			for (UpdateMap::Iter i = updaters->begin(), e = updaters->end(); i != e; ++i) {
				VTableUpdater *u = i.v();
				VTableSlot slot = u->slot();
				GcBitset *found = slot.type == VTableSlot::tStorm ? stormFound : cppFound;

				if (slot.offset >= found->count())
					continue;
				// Already found an overriding function?
				if (found->has(slot.offset))
					continue;

				// Try to find one ourselves...
				if (!hasOverride(slot)) {
					// Disable vtable lookup for this function as it was a leaf function.
					Function *f = i.k();
					if (f->ref().address() != f->directRef().address()) {
						f->setLookup(null);
					}
				}

				// Note we found this one at least!
				found->set(slot.offset, true);
			}
		}

		// Ask our parent to do the same.
		if (Type *s = owner->super())
			rawVTable(s)->disableLookup(cppFound, stormFound);
	}

	Bool VTable::hasOverride(VTableSlot slot) {
		TypeChain::Iter i = owner->chain->children();
		while (Type *c = i.next()) {
			VTable *child = rawVTable(c);

			if (child->get(slot) != null)
				return true;

			if (child->hasOverride(slot))
				return true;
		}

		return false;
	}

	void VTable::slotMoved(VTableSlot slot, const void *addr) {
		set(slot, addr);

		// Traverse the types and update all child types.
		if (!owner->chain)
			return;

		TypeChain::Iter i = owner->chain->children();
		while (Type *c = i.next()) {
			if (VTable *t = rawVTable(c))
				t->parentSlotMoved(slot, addr);
		}
	}

	void VTable::parentSlotMoved(VTableSlot slot, const void *addr) {
		// We do not need to propagate further.
		if (get(slot) != null)
			return;

		set(slot, addr);

		if (!owner->chain)
			return;

		TypeChain::Iter i = owner->chain->children();
		while (Type *c = i.next()) {
			if (VTable *t = rawVTable(c))
				t->parentSlotMoved(slot, addr);
		}
	}


	VTableSlot VTable::findSlot(Function *fn, bool setLookup) {
		Code *c = fn->getCode();
		bool cppType = (owner->typeFlags() & typeCpp) == typeCpp;
		if (cppType && (as<StaticCode>(c) || as<CppMemberFunction>(fn))) {
			// Might be a function from C++!
			const void *addr = fn->originalPtr();
			nat r = cpp->findSlot(addr);
			if (r != vtable::invalid) {
				VTableSlot slot = cppSlot(r);
				if (setLookup)
					updateSuper(slot);
				return slot;
			}
		}

		nat r = vtable::invalid;
		if (storm)
			r = storm->findSlot(fn);

		if (r != vtable::invalid)
			return stormSlot(r);

		// See if our parent has a match!
		if (Type *s = owner->super()) {
			return rawVTable(s)->findSuperSlot(new (this) OverridePart(fn), setLookup);
		} else {
			return VTableSlot();
		}
	}

	VTableSlot VTable::findSuperSlot(OverridePart *fn, bool setLookup) {
		Function *found = as<Function>(owner->findHere(fn, Scope()));
		if (found) {
			// If it is not known to us yet, we can not do much about it.
			if (updaters->has(found)) {
				VTableSlot slot = updaters->get(found)->slot();
				if (setLookup)
					useLookup(found, slot);
				return slot;
			}
		}

		// Try to find it in superclasses.
		if (Type *s = owner->super()) {
			return rawVTable(s)->findSuperSlot(fn, setLookup);
		} else {
			return VTableSlot();
		}
	}

	void VTable::updateSuper(VTableSlot slot) {
		Type *s = owner->super();
		if (!s)
			return;

		VTable *super = rawVTable(s);
		if (Function *fn = get(slot)) {
			useLookup(fn, slot);
		} else {
			// Keep on looking!
			super->updateSuper(slot);
		}
	}

	Bool VTable::updateChildren(Function *fn, VTableSlot slot) {
		if (!owner->chain)
			return false;

		OverridePart *p = new (this) OverridePart(fn);

		bool found = false;
		TypeChain::Iter i = owner->chain->children();
		while (Type *c = i.next())
			found |= rawVTable(c)->updateChildren(p, slot);

		return found;
	}

	Bool VTable::updateChildren(OverridePart *fn, VTableSlot slot) {
		bool found = false;

		// Note: Do not attempt to eagerly load functions in the child class. Functions there will
		// be loaded when the class is created, and we will be notified when that happens.
		if (Function *f = as<Function>(owner->tryFindHere(fn, Scope()))) {
			// If it is not known to us, we can not do anything useful with it.
			if (VTableUpdater *u = updaters->get(f, null)) {
				VTableSlot from = u->slot();
				// If we do not need to change the slot, we do not need to recurse anymore. No other
				// overriding functions need to be updated.
				if (from == slot)
					return true;
				found = true;

				// Otherwise, we need to change slot for this function:
				set(slot, f);
				u->slot(slot);
				updateLookup(f, slot);
				clear(from);
			}
		}

		// Go on recursing!
		TypeChain::Iter i = owner->chain->children();
		while (Type *c = i.next())
			found |= rawVTable(c)->updateChildren(fn, slot);
		return found;
	}

	VTableSlot VTable::allocSlot() {
		if (!storm) {
			assert(false, L"Can not allocate slots for the C++ class " + ::toS(owner->name) +
				L". Are you trying to override a non-virtual function?");
			return VTableSlot();
		}

		nat slot = storm->freeSlot(stormFirst);
		if (slot >= storm->count()) {
			nat oldCount = storm->count();

			// We need to resize 'storm'...
			storm->resize(oldCount + stormGrow);

			// Grow all child vtables as well.
			TypeChain::Iter i = owner->chain->children();
			while (Type *c = i.next())
				rawVTable(c)->parentGrown(oldCount, stormGrow);
		}

		return stormSlot(slot);
	}

	void VTable::parentGrown(Nat pos, Nat count) {
		if (storm)
			storm->insert(pos, count);
		if (pos >= stormFirst)
			stormFirst += count;

		TypeChain::Iter i = owner->chain->children();
		while (Type *c = i.next())
			rawVTable(c)->parentGrown(pos, count);

		for (UpdateMap::Iter i = updaters->begin(), e = updaters->end(); i != e; ++i) {
			Function *f = i.k();

			VTableUpdater *u = i.v();
			VTableSlot slot = u->slot();

			// Relevant to update?
			if (slot.type != VTableSlot::tStorm || slot.offset < pos)
				continue;

			// Update!
			VTableSlot to = slot;
			to.offset += count;
			set(to, f);
			u->slot(to);
			updateLookup(f, to);
			clear(slot);
		}
	}

	void VTable::useLookup(Function *fn, VTableSlot slot) {
		code::RefSource *src = fn->engine().vtableCalls()->get(slot, fn->result);
		fn->setLookup(new (fn) DelegatedCode(code::Ref(src)));
	}

	void VTable::updateLookup(Function *fn, VTableSlot slot) {
		if (fn->ref().address() != fn->directRef().address())
			useLookup(fn, slot);
	}

	void VTable::set(VTableSlot slot, Function *fn) {
		switch (slot.type) {
		case VTableSlot::tCpp:
			cpp->set(slot.offset, fn);
			break;
		case VTableSlot::tStorm:
			if (storm)
				storm->set(slot.offset, fn);
			break;
		default:
			assert(false, L"Unknown slot type.");
			break;
		}
	}

	void VTable::set(VTableSlot slot, const void *addr) {
		switch (slot.type) {
		case VTableSlot::tCpp:
			cpp->set(slot.offset, addr);
			break;
		case VTableSlot::tStorm:
			if (storm)
				storm->set(slot.offset, addr);
			break;
		default:
			assert(false, L"Unknown slot type.");
			break;
		}
	}

	Function *VTable::get(VTableSlot slot) {
		switch (slot.type) {
		case VTableSlot::tCpp:
			return cpp->get(slot.offset);
		case VTableSlot::tStorm:
			if (storm)
				return storm->get(slot.offset);
			else
				return null;
		default:
			assert(false, L"Unknown slot type.");
			return null;
		}
	}

	void VTable::clear(VTableSlot slot) {
		switch (slot.type) {
		case VTableSlot::tCpp:
			cpp->clear(slot.offset);
			break;
		case VTableSlot::tStorm:
			storm->clear(slot.offset);
			break;
		default:
			assert(false, L"Unknown slot type.");
			break;
		}
	}

}
