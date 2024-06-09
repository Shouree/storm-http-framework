#pragma once
#include "Variable.h"
#include "Code/TypeDesc.h"

namespace storm {
	STORM_PKG(core.lang);

	/**
	 * Layout of variables inside a type.
	 *
	 * Currently, allocates the types in the order they were added to the layout, but in the future
	 * it is probably desirable to order the members in increasing size or similar to compact the
	 * layout as much as possible while keeping alignment requirements.
	 */
	class Layout : public ObjectOn<Compiler> {
		STORM_CLASS;
	public:
		// Create.
		STORM_CTOR Layout();

		// Add a variable to the layout.
		void STORM_FN add(MemberVar *var, Type *into);
		void STORM_FN add(Named *n, Type *into);

		// Lay out all variables in memory. Returns the total size of the creation.
		Size STORM_FN doLayout(Size parentSize);

		// Fill in a GcType for the current layout.
		// If 'into' is null, we only compute how many entries we need.
		// 'parent' may be null.
		nat fillGcType(Size parentSize, const GcType *parent, GcType *into);

		// Get a list of all member variables.
		Array<MemberVar *> *variables();

	private:
		// All members in here, in the order they are laid out.
		Array<MemberVar *> *vars;
	};

}
