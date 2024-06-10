#include "stdafx.h"
#include "Operand.h"
#include "Exception.h"
#include "Core/StrBuf.h"
#include "Core/Str.h"
#include <iomanip>

namespace code {

	static Word dual(Nat a, Nat b) {
		return Word(a) << Word(32) | Word(b);
	}

	static Nat firstDual(Word d) {
		return Nat((d >> 32) & 0xFFFFFFFF);
	}

	static Nat secondDual(Word d) {
		return Nat(d & 0xFFFFFFFF);
	}

	using runtime::someEngine;

	Operand::Operand() : opType(opNone), opPtr(null), opNum(0) {}

	Operand::Operand(Reg r) : opType(opRegister), opPtr(null), opNum(r), opSize(code::size(r)) {
		if (r == noReg)
			throw new (someEngine()) InvalidValue(S("noReg is not a proper register!"));
	}

	Operand::Operand(CondFlag c) : opType(opCondFlag), opPtr(null), opNum(c), opSize() {}

	Operand::Operand(Block b) : opType(opBlock), opPtr(null), opNum(b.id), opSize() {
		if (b == Block())
			throw new (someEngine()) InvalidValue(S("Can not create an operand of an empty block."));
	}

	Operand::Operand(Var v) : opType(opVariable), opPtr(null), opNum(v.id), opSize(v.size()) {
		if (v == Var())
			throw new (someEngine()) InvalidValue(S("Can not create an operand of an empty variable."));
	}

	Operand::Operand(Label l) : opType(opLabel), opPtr(null), opNum(l.id), opSize(Size::sPtr) {
		if (l == Label())
			throw new (someEngine()) InvalidValue(S("Can not create an operand of an empty label."));
	}

	Operand::Operand(Ref ref) : opType(opReference), opPtr(ref.to), opNum(0), opSize(Size::sPtr) {}

	Operand::Operand(Reference *ref) : opType(opReference), opPtr(ref->to), opNum(0), opSize(Size::sPtr) {}

	Operand::Operand(SrcPos pos) : opType(opSrcPos), opPtr(pos.file), opNum(), opSize() {
		opNum = Word(pos.start) | (Word(pos.end) << 32);
	}

	Operand::Operand(Word c, Size size) : opType(opConstant), opPtr(null), opNum(c), opSize(size) {}

	Operand::Operand(Size s, Size size) : opType(opDualConstant), opPtr(null), opOffset(s), opSize(size) {}

	Operand::Operand(Offset o, Size size) : opType(opDualConstant), opPtr(null), opOffset(o), opSize(size) {}

	Operand::Operand(RootObject *obj) : opType(opObjReference), opPtr(obj), opOffset(), opSize(Size::sPtr) {}

	Operand::Operand(Reg r, Offset offset, Size size) : opType(opRelative), opPtr(null), opNum(r), opOffset(offset), opSize(size) {
		// Note: it is OK if 'r == noReg'. That means absolute addressing.
	}

	Operand::Operand(Var v, Offset offset, Size size) : opType(opVariable), opPtr(null), opNum(v.id), opOffset(offset), opSize(size) {
		if (v == Var())
			throw new (someEngine()) InvalidValue(S("Can not create an operand of an empty variable."));
	}

	Operand::Operand(Label l, Offset offset, Size size) : opType(opRelativeLbl), opPtr(null), opNum(l.id), opOffset(offset), opSize(size) {
		if (l == Label())
			throw new (someEngine()) InvalidValue(S("Can not create an operand of an empty label."));
	}

	Bool Operand::operator ==(const Operand &o) const {
		if (opType != o.opType)
			return false;

		switch (opType) {
		case opNone:
			return true;
		case opConstant:
		case opRegister:
		case opCondFlag:
		case opBlock:
		case opLabel:
			return opNum == o.opNum;
		case opDualConstant:
			return opOffset == o.opOffset;
		case opVariable:
		case opRelative:
		case opRelativeLbl:
			return opNum == o.opNum && opOffset == o.opOffset;
		case opReference:
		case opObjReference:
			return opPtr == o.opPtr;
		case opSrcPos:
			if (opNum != o.opNum)
				return false;
			if (opPtr == null && o.opPtr == null)
				return true;
			if (opPtr == null || o.opPtr == null)
				return false;
			return *(Url *)opPtr == *(Url *)o.opPtr;
		default:
			assert(false, L"Unknown type!");
			return false;
		}
	}

	Bool Operand::operator !=(const Operand &o) const {
		return !(*this == o);
	}

	Bool Operand::empty() const {
		return opType == opNone;
	}

	Bool Operand::any() const {
		return !empty();
	}

	OpType Operand::type() const {
		if (opType == opDualConstant)
			return opConstant;
		else
			return opType;
	}

	Size Operand::size() const {
		return opSize;
	}

	Bool Operand::readable() const {
		switch (type()) {
		case opNone:
		case opCondFlag:
		case opBlock:
		case opSrcPos:
			// TODO: Add more returning false here.
			return false;
		default:
			return true;
		}
	}

	Bool Operand::writable() const {
		switch (type()) {
		case opRegister:
		case opRelative:
		case opVariable:
			return true;
		default:
			return false;
		}
	}

	void Operand::ensureReadable(op::OpCode op) const {
		if (!readable()) {
			Engine &e = someEngine();
			Str *msg = TO_S(e, S("For instruction ") << name(op) << S(": ") << *this << S(" is not readable."));
			throw new (e) InvalidValue(msg);
		}
	}

	void Operand::ensureWritable(op::OpCode op) const {
		if (!writable()) {
			Engine &e = someEngine();
			Str *msg = TO_S(e, S("For instruction ") << name(op) << S(": ") << *this << S(" is not writable."));
			throw new (e) InvalidValue(msg);
		}
	}

	Bool Operand::hasRegister() const {
		switch (opType) {
		case opRegister:
		case opRelative:
			return true;
		default:
			return false;
		}
	}

	Word Operand::constant() const {
		if (type() != opConstant)
			throw new (someEngine()) InvalidValue(S("Not a constant!"));
		if (opType == opConstant) {
			return opNum;
		} else {
			return opOffset.current();
		}
	}

	Reg Operand::reg() const {
		if (!hasRegister())
			throw new (someEngine()) InvalidValue(S("Not a register!"));
		return Reg(opNum);
	}

	Offset Operand::offset() const {
		if (type() == opDualConstant)
			return Offset();

		// Nothing bad happens if this is accessed wrongly.
		return opOffset;
	}

	CondFlag Operand::condFlag() const {
		if (type() != opCondFlag)
			throw new (someEngine()) InvalidValue(S("Not a CondFlag!"));
		return CondFlag(opNum);
	}

	Block Operand::block() const {
		if (type() != opBlock)
			throw new (someEngine()) InvalidValue(S("Not a block!"));
		return Block(Nat(opNum));
	}

	Var Operand::var() const {
		if (type() != opVariable)
			throw new (someEngine()) InvalidValue(S("Not a variable!"));
		return Var(Nat(opNum), size());
	}

	Ref Operand::ref() const {
		if (type() != opReference)
			throw new (someEngine()) InvalidValue(S("Not a reference!"));
		RefSource *s = (RefSource *)opPtr;
		return Ref(s);
	}

	RefSource *Operand::refSource() const {
		if (type() != opReference)
			throw new (someEngine()) InvalidValue(S("Not a reference!"));
		return (RefSource *)opPtr;
	}

	RootObject *Operand::object() const {
		if (type() != opObjReference)
			throw new (someEngine()) InvalidValue(S("Not an object reference!"));
		return (RootObject *)opPtr;
	}

	MAYBE(Object *) Operand::obj() const {
		if (type() != opObjReference)
			return null;
		return as<Object>((RootObject *)opPtr);
	}

	MAYBE(TObject *) Operand::tObj() const {
		if (type() != opObjReference)
			return null;
		return as<TObject>((RootObject *)opPtr);
	}

	Label Operand::label() const {
		if (type() != opLabel && type() != opRelativeLbl)
			throw new (someEngine()) InvalidValue(S("Not a label!"));
		return Label(Nat(opNum));
	}

	SrcPos Operand::srcPos() const {
		if (type() != opSrcPos)
			throw new (someEngine()) InvalidValue(S("Not a SrcPos!"));
		Nat start = opNum & 0xFFFFFFFF;
		Nat end = opNum >> 32;
		return SrcPos((Url *)opPtr, start, end);
	}

	Operand Operand::replaceRegister(Reg replace) const {
		if (type() == opRegister)
			return Operand(asSize(replace, size()));
		else if (type() == opRelative)
			return Operand(asSize(replace, Size::sPtr), opOffset, opSize);
		else
			return *this;
	}

	wostream &operator <<(wostream &to, const Operand &o) {
		if (o.type() != opRegister
			&& o.type() != opCondFlag
			&& o.type() != opNone
			&& o.type() != opBlock
			&& o.type() != opSrcPos) {

			Size s = o.size();
			if (s == Size::sPtr)
				to << L"p";
			else if (s == Size::sByte)
				to << L"b";
			else if (s == Size::sInt)
				to << L"i";
			else if (s == Size::sLong)
				to << L"l";
			else
				to << L"(" << s << L")";
		}

		switch (o.type()) {
		case opNone:
			return to << L"<none>";
		case opConstant:
			if (o.opType == opDualConstant) {
				return to << o.opOffset;
			} else {
				if (o.size() == Size::sPtr) {
					return to << L"0x" << toHex(o.constant());
				} else {
					return to << o.constant();
				}
			}
		case opRegister:
			return to << code::name(o.reg());
		case opRelative:
			return to << L"[" << code::name(o.reg()) << o.offset() << L"]";
		case opRelativeLbl:
			return to << L"[" << L"Label" << o.opNum << o.offset() << L"]";
		case opVariable:
			if (o.offset() != Offset()) {
				return to << L"[Var" << o.opNum << o.offset() << L"]";
			} else {
				return to << L"[Var" << o.opNum << L"]";
			}
		case opLabel:
			return to << L"Label" << o.opNum;
		case opBlock:
			return to << L"Block" << o.opNum;
		case opReference:
			return to << L"@" << o.ref().title();
		case opObjReference:
			TODO(L"Make sure to call on the correct OS thread!");
			if (o.object()) {
				return to << L"&" << o.object()->toS()->c_str();
			} else {
				return to << L"&null";
			}
		case opCondFlag:
			return to << code::name(o.condFlag());
		case opSrcPos:
			return to << o.srcPos();
		default:
			assert(false, L"Unknown type!");
			return to << L"<invalid>";
		}
	}

	void Operand::toS(StrBuf *to) const {
		if (type() != opRegister
			&& type() != opCondFlag
			&& type() != opNone
			&& type() != opBlock
			&& type() != opSrcPos) {

			Size s = size();
			if (s == Size::sPtr)
				*to << S("p");
			else if (s == Size::sByte)
				*to << S("b");
			else if (s == Size::sInt)
				*to << S("i");
			else if (s == Size::sLong)
				*to << S("l");
			else
				*to << S("(") << s << S(")");
		}

		switch (type()) {
		case opNone:
			*to << S("<none>");
			return;
		case opConstant:
			if (opType == opDualConstant) {
				*to << opOffset;
			} else {
				if (size() == Size::sPtr) {
					*to << S("0x") << hex(constant());
				} else {
					*to << constant();
				}
			}
			return;
		case opRegister:
			*to << code::name(reg());
			return;
		case opRelative:
			*to << S("[") << code::name(reg()) << offset() << S("]");
			return;
		case opRelativeLbl:
			*to << S("[") << L"Label" << opNum << offset() << S("]");
			return;
		case opVariable:
			if (offset() != Offset()) {
				*to << S("[Var") << opNum << offset() << S("]");
			} else {
				*to << S("[Var") << opNum << S("]");
			}
			return;
		case opLabel:
			*to << S("Label") << opNum;
			return;
		case opBlock:
			*to << S("Block") << opNum;
			return;
		case opReference:
			*to << S("@") << ref().title();
			return;
		case opObjReference:
			TODO(L"Make sure to call on the correct OS thread!");
			if (object()) {
				*to << S("&") << object()->toS();
			} else {
				*to << S("&null");
			}
			return;
		case opCondFlag:
			*to << code::name(condFlag());
			return;
		case opSrcPos:
			*to << srcPos();
			return;
		default:
			*to << S("<invalid>");
			return;
		}
	}

	void Operand::dbg_dump() const {
		const nat *raw = (const nat *)this;
		for (nat i = 0; i < sizeof(Operand)/4; i++) {
			std::wcout << std::setw(2) << i << L": " << toHex(raw[i]) << endl;
		}
	}


	/**
	 * Create things:
	 */

	Operand byteConst(Byte v) {
		return xConst(Size::sByte, Word(v));
	}

	Operand intConst(Int v) {
		return xConst(Size::sInt, Long(v));
	}

	Operand intConst(Offset v) {
		return Operand(v, Size::sInt);
	}

	Operand natConst(Nat v) {
		return xConst(Size::sNat, Word(v));
	}

	Operand natConst(Size v) {
		return Operand(v, Size::sNat);
	}

	Operand longConst(Long v) {
		return xConst(Size::sLong, v);
	}

	Operand wordConst(Word v) {
		return xConst(Size::sWord, v);
	}

	Operand floatConst(Float v) {
		union {
			Nat i;
			Float f;
		} convert;
		convert.f = v;
		return xConst(Size::sFloat, convert.i);
	}

	Operand doubleConst(Double v) {
		union {
			Word i;
			Double f;
		} convert;
		convert.f = v;
		return xConst(Size::sDouble, convert.i);
	}

	Operand ptrConst(Size v) {
		return Operand(v, Size::sPtr);
	}

	Operand ptrConst(Offset v) {
		return Operand(v, Size::sPtr);
	}

	Operand ptrConst(Nat v) {
		return xConst(Size::sPtr, v);
	}

	Operand xConst(Size s, Word v) {
		return Operand(v, s);
	}

	Operand objPtr(Object *o) {
		return Operand(o);
	}

	Operand objPtr(TObject *o) {
		return Operand(o);
	}

	Operand byteRel(Reg reg, Offset offset) {
		return xRel(Size::sByte, reg, offset);
	}

	Operand intRel(Reg reg, Offset offset) {
		return xRel(Size::sInt, reg, offset);
	}

	Operand longRel(Reg reg, Offset offset) {
		return xRel(Size::sLong, reg, offset);
	}

	Operand floatRel(Reg reg, Offset offset) {
		return xRel(Size::sFloat, reg, offset);
	}

	Operand doubleRel(Reg reg, Offset offset) {
		return xRel(Size::sDouble, reg, offset);
	}

	Operand ptrRel(Reg reg, Offset offset) {
		return xRel(Size::sPtr, reg, offset);
	}

	Operand xRel(Size size, Reg reg, Offset offset) {
		return Operand(reg, offset, size);
	}

	Operand byteRel(Var v, Offset offset) {
		return xRel(Size::sByte, v, offset);
	}

	Operand intRel(Var v, Offset offset) {
		return xRel(Size::sInt, v, offset);
	}

	Operand longRel(Var v, Offset offset) {
		return xRel(Size::sLong, v, offset);
	}

	Operand floatRel(Var v, Offset offset) {
		return xRel(Size::sFloat, v, offset);
	}

	Operand doubleRel(Var v, Offset offset) {
		return xRel(Size::sDouble, v, offset);
	}

	Operand ptrRel(Var v, Offset offset) {
		return xRel(Size::sPtr, v, offset);
	}

	Operand xRel(Size size, Var v, Offset offset) {
		return Operand(v, offset, size);
	}

	Operand byteRel(Label l, Offset offset) {
		return xRel(Size::sByte, l, offset);
	}

	Operand intRel(Label l, Offset offset) {
		return xRel(Size::sInt, l, offset);
	}

	Operand longRel(Label l, Offset offset) {
		return xRel(Size::sLong, l, offset);
	}

	Operand floatRel(Label l, Offset offset) {
		return xRel(Size::sFloat, l, offset);
	}

	Operand doubleRel(Label l, Offset offset) {
		return xRel(Size::sDouble, l, offset);
	}

	Operand ptrRel(Label l, Offset offset) {
		return xRel(Size::sPtr, l, offset);
	}

	Operand xRel(Size size, Label l, Offset offset) {
		return Operand(l, offset, size);
	}


	Operand byteRel(Reg reg) {
		return xRel(Size::sByte, reg, Offset());
	}

	Operand intRel(Reg reg) {
		return xRel(Size::sInt, reg, Offset());
	}

	Operand longRel(Reg reg) {
		return xRel(Size::sLong, reg, Offset());
	}

	Operand floatRel(Reg reg) {
		return xRel(Size::sFloat, reg, Offset());
	}

	Operand doubleRel(Reg reg) {
		return xRel(Size::sDouble, reg, Offset());
	}

	Operand ptrRel(Reg reg) {
		return xRel(Size::sPtr, reg, Offset());
	}

	Operand xRel(Size size, Reg reg) {
		return xRel(size, reg, Offset());
	}

	Operand byteRel(Var v) {
		return xRel(Size::sByte, v, Offset());
	}

	Operand intRel(Var v) {
		return xRel(Size::sInt, v, Offset());
	}

	Operand longRel(Var v) {
		return xRel(Size::sLong, v, Offset());
	}

	Operand floatRel(Var v) {
		return xRel(Size::sFloat, v, Offset());
	}

	Operand doubleRel(Var v) {
		return xRel(Size::sDouble, v, Offset());
	}

	Operand ptrRel(Var v) {
		return xRel(Size::sPtr, v, Offset());
	}

	Operand xRel(Size size, Var v) {
		return xRel(size, v, Offset());
	}

	Operand byteRel(Label l) {
		return xRel(Size::sByte, l, Offset());
	}

	Operand intRel(Label l) {
		return xRel(Size::sInt, l, Offset());
	}

	Operand longRel(Label l) {
		return xRel(Size::sLong, l, Offset());
	}

	Operand floatRel(Label l) {
		return xRel(Size::sFloat, l, Offset());
	}

	Operand doubleRel(Label l) {
		return xRel(Size::sDouble, l, Offset());
	}

	Operand ptrRel(Label l) {
		return xRel(Size::sPtr, l, Offset());
	}

	Operand xRel(Size size, Label l) {
		return xRel(size, l, Offset());
	}

}
