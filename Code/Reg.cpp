#include "stdafx.h"
#include "Reg.h"
#include "X86/Asm.h"
#include "X64/Asm.h"
#include "Arm64/Asm.h"
#include "Core/StrBuf.h"

namespace code {

	const wchar *name(Reg r) {
		switch (r) {
		case noReg:
			return S("<none>");
		case ptrStack:
			return S("ptrStack");
		case ptrFrame:
			return S("ptrFrame");
		case ptrA:
			return S("ptrA");
		case ptrB:
			return S("ptrB");
		case ptrC:
			return S("ptrC");
		case al:
			return S("al");
		case bl:
			return S("bl");
		case cl:
			return S("cl");
		case eax:
			return S("eax");
		case ebx:
			return S("ebx");
		case ecx:
			return S("ecx");
		case rax:
			return S("rax");
		case rbx:
			return S("rbx");
		case rcx:
			return S("rcx");
		default:
			if (const wchar *n = x86::nameX86(r))
				return n;
			if (const wchar *n = x64::nameX64(r))
				return n;
			if (const wchar *n = arm64::nameArm64(r))
				return n;
			assert(false, L"Unknown register: " + ::toS(r));
			return S("<unknown>");
		}
	}

	Size size(Reg r) {
		nat v = (nat)r;
		switch ((v & 0xF00) >> 8) {
		case 0:
			return Size::sPtr;
		case 1:
			return Size::sByte;
		case 4:
			return Size::sNat;
		case 8:
			return Size::sLong;
		default:
			assert(false, L"Unknown size for register: " + ::toS(name(r)));
			return Size::sPtr;
		}
	}

	Reg asSize(Reg r, Size size) {
		if (r == noReg)
			return noReg;

		nat s = 0;
		if (size == Size::sPtr) {
			s = 0;
		} else if (size == Size::sByte) {
			s = 1;
		} else if (size == Size::sNat) {
			s = 4;
		} else if (size == Size::sLong) {
			s = 8;
		} else {
			return noReg;
		}

		nat v = (nat)r;
		return Reg((v & ~0xF00) | s << 8);
	}

	Bool same(Reg a, Reg b) {
		nat aa = (nat)a;
		nat bb = (nat)b;
		return (aa & 0xFF) == (bb & 0xFF);
	}

	static const Reg gpRegs[] = { ptrA, ptrB, ptrC };

	Reg freeReg(Reg a) {
		for (nat i = 0; i < ARRAY_COUNT(gpRegs); i++)
			if (!same(a, gpRegs[i]))
				return gpRegs[i];
		return noReg;
	}

	Reg freeReg(Reg a, Reg b) {
		for (nat i = 0; i < ARRAY_COUNT(gpRegs); i++)
			if (!same(a, gpRegs[i]) && !same(b, gpRegs[i]))
				return gpRegs[i];
		return noReg;
	}


	RegSet::RegSet() {}

	RegSet::RegSet(const RegSet &o) {
		index = o.index;
		for (nat i = 0; i < banks; i++)
			(&data0)[i] = (&o.data0)[i];
		numSet = o.numSet;
	}

	RegSet::RegSet(Reg r) {
		put(r);
	}

	RegSet::RegSet(Array<Reg> *regs) {
		for (nat i = 0; i < regs->count(); i++)
			put(regs->at(i));
	}

	void RegSet::deepCopy(CloneEnv *env) {
		// No need.
	}

	Bool RegSet::has(Reg r) const {
		nat bank = findBank(registerBackend(r));
		if (bank >= banks)
			return false;

		return readData(bank, registerSlot(r)) != 0;
	}

	void RegSet::put(Reg r) {
		if (r == ptrStack || r == ptrFrame || r == noReg)
			return;

		nat bank = findBank(registerBackend(r));
		if (bank >= banks)
			bank = allocBank(registerBackend(r));

		nat slot = registerSlot(r);
		nat old = readData(bank, slot);
		if (old == 0)
			numSet++;
		nat size = max(old, registerSize(r));
		writeData(bank, slot, size);
	}

	void RegSet::put(const RegSet *src) {
		for (Iter i = src->begin(); i != src->end(); ++i)
			put(*i);
	}

	Reg RegSet::get(Reg r) const {
		nat bank = findBank(registerBackend(r));
		if (bank >= banks)
			return noReg;

		return readRegister(bank, registerSlot(r));
	}

	void RegSet::remove(Reg r) {
		nat bank = findBank(registerBackend(r));
		if (bank >= banks)
			return;

		nat slot = registerSlot(r);
		if (readData(bank, slot) != 0 && numSet > 0)
			numSet--;
		writeData(bank, slot, 0);
		if (emptyBank(bank) && bank > 0)
			writeIndex(bank, 0);
	}

	void RegSet::clear() {
		index = 0;
		for (nat i = 0; i < banks; i++) {
			(&data0)[i] = 0;
		}
	}

	// Can not be implemented right now, as new (this) Array<Reg>() does not work yet.
	// Array<Reg> *RegSet::all() const {
	// 	Array<Reg> *result = new (this) Array<Reg>();

	// 	for (Iter i = begin(); i != end(); ++i)
	// 		result->push(i.v());

	// 	return result;
	// }

	RegSet::Iter::Iter() : owner(null), pos(0) {}

	RegSet::Iter::Iter(const RegSet *reg) : owner(reg), pos(0) {
		if (empty(0))
			++*this;
	}

	Bool RegSet::Iter::operator ==(Iter o) const {
		if (atEnd() || o.atEnd())
			return atEnd() == o.atEnd();
		else
			return owner == o.owner && pos == o.pos;
	}

	Bool RegSet::Iter::operator !=(Iter o) const {
		return !(*this == o);
	}

	RegSet::Iter &RegSet::Iter::operator ++() {
		while (!atEnd()) {
			pos++;
			if (!empty(pos))
				break;
		}

		return *this;
	}

	RegSet::Iter RegSet::Iter::operator ++(int) {
		Iter c = *this;
		++*this;
		return c;
	}

	bool RegSet::Iter::atEnd() const {
		return owner == null || pos >= (dataSlots * banks);
	}

	bool RegSet::Iter::empty(Nat pos) const {
		return owner->readData(pos / dataSlots, pos % dataSlots) == 0;
	}

	Reg RegSet::Iter::read(Nat pos) const {
		return owner->readRegister(pos / dataSlots, pos % dataSlots);
	}

	Reg RegSet::Iter::operator *() const {
		return read(pos);
	}

	Reg RegSet::Iter::v() const {
		return read(pos);
	}

	RegSet::Iter RegSet::begin() const {
		return Iter(this);
	}

	RegSet::Iter RegSet::end() const {
		return Iter();
	}

	void RegSet::toS(StrBuf *to) const {
		bool first = true;
		for (Iter i = begin(); i != end(); ++i) {
			if (!first)
				*to << L", ";
			first = false;

			*to << name(*i);
		}
	}

	Nat RegSet::readIndex(Nat bank) const {
		if (bank == 0)
			return 0;

		nat shift = (bank - 1) * 4;
		return (index >> shift) & 0xF;
	}

	void RegSet::writeIndex(Nat bank, Nat v) {
		assert(bank > 0);
		if (bank == 0)
			return;

		nat shift = (bank - 1) * 4;
		index &= ~(0xF << shift);
		index |= (v & 0xF) << shift;
	}

	Nat RegSet::readData(Nat bank, Nat slot) const {
		const Nat *d = &data0;
		Nat read = d[bank];

		nat shift = slot * 2;
		return (read >> shift) & 0x3;
	}

	void RegSet::writeData(Nat bank, Nat slot, Nat v) {
		Nat *d = &data0;
		Nat &write = d[bank];

		nat shift = slot * 2;
		write &= ~(0x3 << shift);
		write |= (v & 0x3) << shift;
	}

	bool RegSet::emptyBank(Nat bank) const {
		const Nat *d = &data0;
		return d[bank] == 0;
	}

	Nat RegSet::findBank(Nat backend) const {
		if (backend == 0)
			return 0;

		for (nat i = 1; i < banks; i++)
			if (readIndex(i) == backend)
				return i;

		return banks;
	}

	Nat RegSet::allocBank(Nat backend) {
		if (backend == 0)
			return 0;

		for (nat i = 1; i < banks; i++)
			if (readIndex(i) == 0) {
				writeIndex(i, backend);
				return i;
			}

		assert(false, L"Out of space for registers!");
		return 0;
	}

	Reg RegSet::readRegister(Nat bank, Nat slot) const {
		Nat backend = readIndex(bank);
		Nat size = readData(bank, slot);

		static nat lookup[] = { 0x0, 0x4, 0x0, 0x8 };
		if (size == 0)
			return noReg;

		Nat data = (backend & 0xF) << 4;
		data |= (lookup[size] & 0xF) << 8;
		data |= (slot & 0xF);
		return Reg(data);
	}

	Nat RegSet::registerSlot(Reg r) {
		Nat z = r;
		return z & 0xF;
	}

	Nat RegSet::registerBackend(Reg r) {
		Nat z = r;
		return (z & 0xF0) >> 4;
	}

	Nat RegSet::registerSize(Reg r) {
		Nat z = r;
		switch ((z & 0xF00) >> 8) {
		case 0x0:
			return 2;
		case 0x1:
		case 0x4:
			return 1;
		case 0x8:
			return 3;
		default:
			WARNING(L"Unknown size!");
			return 2;
		}
	}

}
