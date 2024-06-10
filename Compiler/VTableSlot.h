#pragma once

namespace storm {
	STORM_PKG(core.lang);

	/**
	 * Description of a slot inside a VTable.
	 */
	class VTableSlot {
		STORM_VALUE;
	public:
		enum VType {
			tNone, tCpp, tStorm
		};

		STORM_CTOR VTableSlot();
		STORM_CTOR VTableSlot(VType t, Nat offset);

		// Type of entry.
		VType type;

		// Offset into the specified vtable.
		Nat offset;

		// A valid slot?
		inline Bool STORM_FN valid() const {
			return type != tNone;
		}

		// Compare.
		Bool STORM_FN operator ==(VTableSlot o) const;
		inline Bool STORM_FN operator !=(VTableSlot o) const { return !(*this == o); }

		// Output.
		void STORM_FN toS(StrBuf *to) const;
	};

	// Helpers:
	VTableSlot STORM_FN cppSlot(Nat id);
	VTableSlot STORM_FN stormSlot(Nat id);

	// To string.
	wostream &operator <<(wostream &to, const VTableSlot &pos);

}
