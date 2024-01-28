#pragma once
#include "Core/TObject.h"
#include "Core/Map.h"
#include "Core/GcBitset.h"
#include "Code/Listing.h"
#include "VTableSlot.h"
#include "VTableCpp.h"
#include "VTableStorm.h"
#include "VTableUpdater.h"
#include "Function.h"
#include "OverridePart.h"

namespace storm {
	STORM_PKG(core.lang);

	class Type;

	/**
	 * The VTable implementation in Storm is split into three files. This one (VTable.h) contains
	 * the logical 'one and only' VTable implementation for a type. However, in Storm a VTable is
	 * split into two parts: the C++ VTable and the Storm VTable.
	 *
	 * The C++ VTable is managed by VTableCpp, and is binary compatible with C++. Storm produces its
	 * own derivatives to be able to extend classes declared in C++, but this has some
	 * limitations. For example, we can not dynamically add slots in the VTable without having to
	 * walk the heap and examine all living objects to see if we need to replace their VTables
	 * whenever the VTable shall be expanded.
	 *
	 * Instead, we store the dynamic part in the Storm VTable, which is managed by VTableStorm. For
	 * types declared in Storm, we store a pointer to the Storm VTable at the unused offset -1 in
	 * the C++ VTable. This allows us to change the Storm VTable for all living objects by simply
	 * altering one pointer. However, this indirection has a small penalty which we might want to
	 * get rid of eventually.
	 *
	 * If a vtable is set in this vtable, it is automatically updated in all vtables of the child
	 * class, as told by 'owner'. The vtable is also responsible for keeping vtable dispatch up to
	 * date if vtable slots are moved. We assume we will be notified when a child has been removed,
	 * and that we will be re-created whenever we gain a new parent.
	 *
	 * Note: As mentioned in the documentation for 'core.lang.Type', we assume that we do not need
	 * to eagerly load functions in child classes since they are not needed until that class is
	 * instantiated, in which case we will be notified. If we would eagerly load subclasses in this
	 * manner, we would create dependencies that are hard to realize.
	 *
	 * TODO: It might be possible to only store a bitmask in VTableCpp and VTableStorm that
	 * indicates if we have a new function set at this level or not.
	 */
	class VTable : public ObjectOn<Compiler> {
		STORM_CLASS;
		friend class VTableUpdater;
	public:
		// Create a VTable for a type. Use 'createXxx()' to supply the C++ vtable we shall be
		// derived from. Needs to know which type we're associated with as we need to traverse the
		// inheritance hierarchy for certain operations.
		STORM_CTOR VTable(Type *owner);

		// Create vtable for a class representing a pure C++ type. In this mode, it is not possible
		// to replace functions in the VTable.
		void createCpp(const void *cppVTable);

		// Create a vtable for a class derived from 'parent'. If done more than once, the current
		// VTable is cleared and then re-initialized.
		void STORM_FN createStorm(VTable *parent);

		// Insert a function to be managed by this VTable. 'fn' will be allocated in a suitable slot
		// and will be set to use a lookup if needed.
		void STORM_FN insert(Function *fn);

		// Notify that a child class has been removed. We assume it is not possible to reach 'type'
		// by traversing the inheritance hierarchy.
		void STORM_FN removeChild(Type *type);

		// Late initialization.
		void lateInit();

		// Set the vtable of an object.
		void insert(RootObject *obj);

		// Generate code for setting the vtable.
		void STORM_FN insert(code::Listing *to, code::Var obj);

		// Get the "topmost" functions for each entry in the vtable. Unused slots are left out
		// (since they would be null).
		Array<Function *> *allSlots();

		// Dump the vtable contents to stdout.
		void dbg_dump() const;

	private:
		// Owning type.
		Type *owner;

		// The derived C++ VTable (might be write-protected).
		VTableCpp *cpp;

		// The derived Storm VTable (if we created one).
		VTableStorm *storm;

		// First slot we have access to in the Storm vtable. This equals the size of our parent's
		// vtable.
		Nat stormFirst;

		// Associate an updater with each function.
		typedef Map<Function *, VTableUpdater *> UpdateMap;
		Map<Function *, VTableUpdater *> *updaters;

		// RefSource referring to the VTable.
		code::RefSource *source;

		// Called when one of our parent classes have grown their vtable and we need to follow.
		void parentGrown(Nat pos, Nat count);

		// Called when we need to update a slot.
		void slotMoved(VTableSlot slot, const void *newAddr);

		// Update slots in all children.
		void parentSlotMoved(VTableSlot slot, const void *newAddr);

		// Find the current vtable slot for 'fn' (if any) in this vtable or in any parent vtables.
		VTableSlot findSlot(Function *fn, Bool setLookup);

		// Find a slot matching 'fn' assuming we're a super class to where 'fn' belongs. If
		// 'setLookup' is true, then use vtable lookup for the most specific match found.
		VTableSlot findSuperSlot(OverridePart *fn, Bool setLookup);

		// Enable lookup for 'slot' in any parent classes.
		void updateSuper(VTableSlot slot);

		// Find (and update) any children overriding 'fn'. Returns true if any child is found.
		Bool updateChildren(Function *fn, VTableSlot slot);

		// Helper to the above function.
		Bool updateChildren(OverridePart *fn, VTableSlot slot);

		// See if some child is using slot 'slot' to override the function in that slot.
		Bool hasOverride(VTableSlot slot);

		// For all slots not already found in the two sets: try to find an overriding function for
		// them. If none is found, disable vtable lookup on the function for that slot.
		void disableLookup(GcBitset *cppFound, GcBitset *stormFound);

		// Set 'slot' to refer to a specific function.
		void set(VTableSlot slot, Function *fn);
		void set(VTableSlot slot, const void *fn);

		// Get the function associated with 'slot'. We allow reading out of bounds.
		MAYBE(Function *) get(VTableSlot slot);

		// Clear 'slot'.
		void clear(VTableSlot slot);

		// Create a slot for a Storm function. Returns the created index.
		VTableSlot allocSlot();

		// Fill in functions for slots into the two arrays.
		void allSlots(Array<Function *> *cppFns, Array<Function *> *stormFns);

		// Use lookup for 'fn'.
		static void useLookup(Function *fn, VTableSlot slot);

		// Update lookup for 'fn' to use 'slot'. Does nothing if no lookup is already used.
		static void updateLookup(Function *fn, VTableSlot slot);

		// Decide how much to grow each time 'storm' needs to grow.
		static const Nat stormGrow = 5;
	};


}
