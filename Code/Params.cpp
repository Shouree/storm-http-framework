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

	Bool Param::empty() {
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

	StrBuf &operator <<(StrBuf &to, Param p) {
		if (p.empty()) {
			to << S("empty");
		} else {
			to << S("#") << p.id() << S("+") << p.offset() << S(",") << p.size();
		}
		if (p.inMemory())
			to << S("(in memory)");
		return to;
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
		for (Nat i = array->filled; i < array->count; i++)
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
		this->stackAlign = stackAlign;
		this->stackParamAlign = stackParamAlign;
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

	void Params::add(Nat id, Primitive p) {
		addPrimitive(id, p);
	}

	void Params::addInt(Param param) {
		if (integer->filled < integer->count)
			integer->v[integer->filled++] = param;
		else
			addStack(param);
	}

	void Params::addReal(Param param) {
		if (real->filled < real->count)
			real->v[real->filled++] = param;
		else
			addStack(param);
	}

	void Params::addStack(Param param) {
		// Align argument properly.
		// Note: parameter size does not matter if we use 32- or 64- version since they are merged inside Param.
		stackSize = roundUp(stackSize, param.size().align64());

		if (!stackPar || stackPar->count == stackPar->filled) {
			Nat newSize = stackPar ? stackPar->filled * 2 : 10;
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
		stackSize += roundUp(param.size().aligned().size64(), stackParamAlign);
	}

	Bool Params::hasInt(Nat space) {
		return integer->filled + space < integer->count;
	}

	Bool Params::hasReal(Nat space) {
		return real->filled + space < real->count;
	}

	void Params::toS(StrBuf *to) const {
		*to << S("Parameters:");
		for (Nat i = 0; i < integer->filled; i++)
			*to << S("\n") << name(registerSrc(i)) << i << S(":") << integer->v[i];
		for (Nat i = 0; i < real->filled; i++)
			*to << S("\n") << name(registerSrc(i + integer->count)) << S(":") << real->v[i];

		if (stackCount() > 0) {
			*to << S("\nOn stack:");
			for (Nat i = 0; i < stackCount(); i++) {
				*to << S(" ") << stackParam(i) << S("@") << stackOffset(i) << S("\n");
			}
		}
	}

	Param Params::totalParam(Nat n) const {
		if (n < integer->count)
			return integer->v[n];
		n -= integer->count;
		if (n < real->count)
			return real->v[n];
		n -= real->count;
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

	StrBuf &operator <<(StrBuf &to, Result r) {
		if (r.memoryRegister() != noReg) {
			to << S("in memory: ") << name(r.memoryRegister());
		} else if (r.registerCount() > 0) {
			for (Nat i = 0; i < r.registerCount(); i++)
				to << name(r.registerAt(i)) << S(": @") << r.registerOffset(i) << S("\n");
		} else {
			to << S("(empty)");
		}
		return to;
	}
}
