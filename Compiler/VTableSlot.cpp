#include "stdafx.h"
#include "VTableSlot.h"
#include "VTableCpp.h"
#include "Core/StrBuf.h"

namespace storm {

	VTableSlot::VTableSlot() : type(tNone), offset(0) {}

	VTableSlot::VTableSlot(VType t, Nat offset) : type(t), offset(offset) {
		if (offset == vtable::invalid)
			type = tNone;
	}

	Bool VTableSlot::operator ==(VTableSlot o) const {
		if (type != o.type)
			return false;

		switch (type) {
		case tNone:
			return true;
		case tStorm:
		case tCpp:
			return offset == o.offset;
		default:
			// Something is wrong...
			return false;
		}
	}

	VTableSlot cppSlot(Nat offset) {
		return VTableSlot(VTableSlot::tCpp, offset);
	}

	VTableSlot stormSlot(Nat offset) {
		return VTableSlot(VTableSlot::tStorm, offset);
	}


	wostream &operator <<(wostream &to, const VTableSlot &pos) {
		switch (pos.type) {
		case VTableSlot::tNone:
			to << L"invalid";
			return to;
		case VTableSlot::tStorm:
			to << L"storm";
			break;
		case VTableSlot::tCpp:
			to << L"c++";
			break;
		}
		to << L":" << pos.offset;
		return to;
	}

	void VTableSlot::toS(StrBuf *to) const {
		switch (type) {
		case VTableSlot::tNone:
			*to << S("invalid");
			return;
		case VTableSlot::tStorm:
			*to << S("storm");
			break;
		case VTableSlot::tCpp:
			*to << S("c++");
			break;
		}
		*to << S(":") << offset;
	}

}
