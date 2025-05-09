#include "stdafx.h"
#include "Params.h"
#include "Exception.h"

namespace code {

	Param::Param() {
		clear();
	}

	Param::Param(Nat id, Primitive p, Bool use64) {
		set(id, p.size(), use64, use64 ? p.offset().v64() : p.offset().v32(), false);
	}

	Param::Param(Nat id, Size size, Bool use64, Nat offset, Bool inMemory) {
		set(id, size, use64, offset, inMemory);
	}

	Param Param::withId(Nat id) {
		Param copy = *this;
		copy.data &= ~Nat(0xFF << 1);
		copy.data |= (Nat(id) & 0xFF) << 1;
		return copy;
	}

	Bool Param::empty() const {
		return data == 0xFFFFFFFE;
	}

	void Param::clear() {
		set(0xFF, Size(), true, 0xFFFFFFFF, false);
	}

	void Param::set(Nat id, Size size, Bool use64, Nat offset, Bool inMemory) {
		if (use64)
			dsize = (size.size64() << 4) | size.align64();
		else
			dsize = (size.size32() << 4) | size.align32();
		data = Nat(inMemory);
		data |= (id & 0xFF) << 1;
		data |= (offset & 0x7FFFFF) << 9;
	}

	wostream &operator <<(wostream &to, Param p) {
		if (p.empty()) {
			to << L"empty";
		} else {
			to << L"#" << p.id() << L"+" << p.offset() << L"," << p.size();
		}
		if (p.inMemory())
			to << L"(in memory)";
		return to;
	}

	void Param::toS(StrBuf *to) const {
		if (empty()) {
			*to << S("empty");
		} else {
			*to << S("#") << id() << S("+") << offset() << S(",") << size();
		}
		if (inMemory())
			*to << S("(in memory)");
	}


	const GcType Params::paramType = {
		GcType::tArray,
		null,
		null,
		sizeof(Param),
		0,
		{},
	};

	const GcType Params::stackParamType = {
		GcType::tArray,
		null,
		null,
		sizeof(StackParam),
		0,
		{},
	};

	static void clear(GcArray<Param> *array) {
		for (size_t i = array->filled; i < array->count; i++)
			array->v[i].clear();
	}

	Params::Params(Nat intCount, Nat realCount, Nat stackParamAlign, Nat stackAlign) {
		this->integer = runtime::allocArray<Param>(engine(), &paramType, intCount);
		this->real = runtime::allocArray<Param>(engine(), &paramType, realCount);
		this->integer->filled = 0;
		this->real->filled = 0;
		clear(this->integer);
		clear(this->real);
		this->stackPar = null;
		this->stackSize = 0;
		this->stackData = (stackAlign & 0xFF) | (stackParamAlign & 0xFF) << 8;
	}

	void Params::result(TypeDesc *type) {
		if (PrimitiveDesc *p = as<PrimitiveDesc>(type)) {
			resultPrimitive(p->v);
		} else if (ComplexDesc *c = as<ComplexDesc>(type)) {
			resultComplex(c);
		} else if (SimpleDesc *s = as<SimpleDesc>(type)) {
			resultSimple(s);
		} else {
			throw new (this) InvalidValue(TO_S(this, S("Unknown type description found: ") << type));
		}
	}

	void Params::result(Primitive p) {
		resultPrimitive(p);
	}

	void Params::add(Nat id, TypeDesc *type) {
		if (PrimitiveDesc *p = as<PrimitiveDesc>(type)) {
			addPrimitive(id, p->v);
		} else if (ComplexDesc *c = as<ComplexDesc>(type)) {
			addComplex(id, c);
		} else if (SimpleDesc *s = as<SimpleDesc>(type)) {
			addSimple(id, s);
		} else {
			throw new (this) InvalidValue(TO_S(this, S("Unknown type description found: ") << type));
		}
	}

	static void bumpFilled(GcArray<Param> *in) {
		while (in->filled < in->count) {
			if (in->v[in->filled].any())
				in->filled++;
			else
				break;
		}
	}

	void Params::add(Nat id, Primitive p) {
		addPrimitive(id, p);
	}

	void Params::addInt(Param param) {
		if (integer->filled < integer->count) {
			integer->v[integer->filled++] = param;
			bumpFilled(integer); // If another parameter was added.
		} else {
			addStack(param);
		}
	}

	void Params::addReal(Param param) {
		if (!unifiedIntFpRegs()) {
			// Normal case:
			if (real->filled < real->count)
				real->v[real->filled++] = param;
			else
				addStack(param);
		} else {
			// Unified case, use 'filled' from 'integer'.
			if (integer->filled < real->count) {
				real->v[integer->filled++] = param;
				bumpFilled(integer);
			} else {
				addStack(param);
			}
		}
	}

	void Params::addStack(Param param) {
		// Align argument properly.
		// Note: parameter size does not matter if we use 32- or 64- version since they are merged inside Param.
		stackSize = roundUp(stackSize, param.size().align64());

		if (!stackPar || stackPar->count == stackPar->filled) {
			Nat newSize = stackPar ? Nat(stackPar->filled) * 2 : 10;
			GcArray<StackParam> *copy = runtime::allocArray<StackParam>(engine(), &stackParamType, newSize);
			copy->filled = 0;
			if (stackPar) {
				memcpy(copy->v, stackPar->v, sizeof(StackParam) * stackPar->filled);
				copy->filled = stackPar->filled;
			}
			stackPar = copy;
		}

		stackPar->v[stackPar->filled].param = param;
		stackPar->v[stackPar->filled].offset = stackSize;
		stackPar->filled++;

		// Update size. Minimum alignment is 8.
		stackSize += roundUp(param.size().aligned().size64(), stackParamAlign());
	}

	void Params::addInt(Nat at, Param param) {
		if (at < integer->count) {
			integer->v[at] = param;
			bumpFilled(integer);
		}
	}

	Bool Params::hasInt(Nat space) {
		return integer->filled + space <= integer->count;
	}

	Bool Params::hasReal(Nat space) {
		return real->filled + space <= real->count;
	}

	void Params::toS(StrBuf *to) const {
		*to << S("Parameters:");
		if (!unifiedIntFpRegs()) {
			for (Nat i = 0; i < integer->filled; i++)
				*to << S("\n") << name(registerSrc(i)) << S(":") << integer->v[i];
			for (Nat i = 0; i < real->filled; i++)
				*to << S("\n") << name(registerSrc(i + Nat(integer->count))) << S(":") << real->v[i];
		} else {
			for (Nat i = 0; i < integer->filled; i++) {
				*to << S("\n");
				if (integer->v[i].any())
					*to << name(registerSrc(i)) << S(":") << integer->v[i];
				else
					*to << name(registerSrc(i + Nat(integer->count))) << S(":") << real->v[i];
			}
		}

		if (stackCount() > 0) {
			*to << S("\nOn stack:");
			for (Nat i = 0; i < stackCount(); i++) {
				*to << S("\n ") << stackParam(i) << S("@");
				Nat offset = stackOffset(i);
				if (offset < 256)
					*to << hex(Byte(offset));
				else
					*to << hex(offset);
			}
		}
	}

	Param Params::totalParam(Nat n) const {
		if (n < integer->count)
			return integer->v[n];
		n -= Nat(integer->count);
		if (n < real->count)
			return real->v[n];
		n -= Nat(real->count);
		if (stackPar && n < stackPar->filled)
			return stackPar->v[n].param;
		assert(false, L"Out of bounds.");
		return Param();
	}


	const GcType Result::dataType = {
		GcType::tArray,
		null,
		null,
		sizeof(Data),
		0,
		{},
	};

	Result::Result() : memReg(noReg), regs(null) {}

	Result Result::inMemory(Reg reg) {
		Result r;
		r.memReg = reg;
		return r;
	}

	Result Result::inRegisters(EnginePtr e, Nat count) {
		Result r;
		r.regs = runtime::allocArray<Data>(e, &dataType, count);
		return r;
	}

	Result Result::inRegister(EnginePtr e, Reg reg) {
		Result r;
		r.regs = runtime::allocArray<Data>(e, &dataType, 1);
		r.regs->v[0].reg = reg;
		r.regs->v[0].offset = 0;
		r.regs->filled = 1;
		return r;
	}

	void Result::putRegister(Reg reg, Nat offset) {
		if (!regs)
			return;
		if (regs->filled < regs->count) {
			regs->v[regs->filled].reg = reg;
			regs->v[regs->filled].offset = offset;
			regs->filled++;
		}
	}

	wostream &operator <<(wostream &to, Result r) {
		if (r.memoryRegister() != noReg) {
			to << L"in memory: " << name(r.memoryRegister());
		} else if (r.registerCount() > 0) {
			for (Nat i = 0; i < r.registerCount(); i++)
				to << name(r.registerAt(i)) << L": @" << r.registerOffset(i) << L"\n";
		} else {
			to << L"(empty)";
		}
		return to;
	}

	void Result::toS(StrBuf *to) const {
		if (memoryRegister() != noReg) {
			*to << S("in memory: ") << name(memoryRegister());
		} else if (registerCount() > 0) {
			for (Nat i = 0; i < registerCount(); i++)
				*to << name(registerAt(i)) << S(": @") << registerOffset(i) << S("\n");
		} else {
			*to << S("(empty)");
		}
	}
}
