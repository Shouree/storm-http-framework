#include "stdafx.h"
#include "VTableCall.h"
#include "Code/Listing.h"
#include "Core/StrBuf.h"
#include "Engine.h"

namespace storm {

	VTableCalls::VTableCalls() {
		cpp = new (this) Array<code::RefSource *>();
		storm = new (this) Array<code::RefSource *>();
		variants = engine().arena()->firstParamId(null);
	}


	code::RefSource *VTableCalls::get(VTableSlot slot, Value result) {
		Engine &e = engine();
		Nat id = e.arena()->firstParamId(result.desc(e));

		switch (slot.type) {
		case VTableSlot::tCpp:
			return getCpp(slot.offset, id);
		case VTableSlot::tStorm:
			return getStorm(slot.offset, id);
		default:
			assert(false, L"Unknown slot type.");
			return null;
		}
	}

	code::RefSource *&VTableCalls::find(Array<code::RefSource *> *in, Nat offset, Nat id) {
		Nat arrayId = offset*variants + id;
		while (in->count() <= arrayId)
			in->push(null);

		return in->at(arrayId);
	}


	code::RefSource *VTableCalls::getCpp(Nat offset, Nat id) {
		code::RefSource *&entry = find(cpp, offset, id);
		if (entry)
			return entry;

		using namespace code;
		Reg tmpReg = engine().arena()->functionDispatchReg();
		Reg firstReg = tmpReg;
		Operand firstParam = engine().arena()->firstParamLoc(id);

		Listing *l = new (this) Listing();
		if (firstParam.type() == opRegister) {
			firstReg = firstParam.reg();
		} else {
			*l << mov(tmpReg, firstParam);
			firstReg = tmpReg;
		}

		*l << mov(tmpReg, ptrRel(firstReg, Offset()));
		*l << jmp(ptrRel(tmpReg, Offset::sPtr * offset));

		Binary *b = new (this) Binary(engine().arena(), l);
		entry = new (this) VTableSource(cppSlot(offset), id, b);
		return entry;
	}

	code::RefSource *VTableCalls::getStorm(Nat offset, Nat id) {
		code::RefSource *&entry = find(storm, offset, id);
		if (entry)
			return entry;

		using namespace code;
		Reg tmpReg = engine().arena()->functionDispatchReg();
		Reg firstReg = tmpReg;
		Operand firstParam = engine().arena()->firstParamLoc(id);

		Listing *l = new (this) Listing();
		if (firstParam.type() == opRegister) {
			firstReg = firstParam.reg();
		} else {
			*l << mov(tmpReg, engine().arena()->firstParamLoc(id));
			firstReg = tmpReg;
		}

		*l << mov(tmpReg, ptrRel(firstReg, Offset()));
		*l << mov(tmpReg, ptrRel(tmpReg, -Offset::sPtr * vtable::extraOffset));
		*l << jmp(ptrRel(tmpReg, Offset::sPtr * (offset + 2))); // 2 for the 2 size_t members in arrays.

		Binary *b = new (this) Binary(engine().arena(), l);
		entry = new (this) VTableSource(stormSlot(offset), id, b);
		return entry;
	}

	VTableSource::VTableSource(VTableSlot slot, Nat id, code::Content *c) : RefSource(c), slot(slot), id(id) {}

	Str *VTableSource::title() const {
		StrBuf *out = new (this) StrBuf();
		*out << S("vtable ") << slot << S(",") << id;
		return out->toS();
	}

}
