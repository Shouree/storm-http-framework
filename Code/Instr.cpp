#include "stdafx.h"
#include "Instr.h"
#include "Exception.h"
#include "Reg.h"
#include "Core/StrBuf.h"
#include "Core/CloneEnv.h"

namespace code {

	Instr::Instr() : iOp(op::nop) {}

	Instr::Instr(const Instr &o) : iOp(o.iOp), iSrc(o.iSrc), iDest(o.iDest) {}

	Instr::Instr(op::OpCode op, const Operand &dest, const Operand &src) : iOp(op), iDest(dest), iSrc(src) {}

	void Instr::deepCopy(CloneEnv *env) {
		// Everything is constant here anyway. No need!
	}

	Size Instr::size() const {
		return max(iSrc.size(), iDest.size());
	}

	op::OpCode Instr::op() const {
		return iOp;
	}

	DestMode Instr::mode() const {
		return destMode(iOp);
	}

	Operand Instr::src() const {
		return iSrc;
	}

	Operand Instr::dest() const {
		return iDest;
	}

	Instr *Instr::alter(Operand dest, Operand src) {
		return new (this) Instr(iOp, dest, src);
	}

	Instr *Instr::alterSrc(Operand src) {
		return new (this) Instr(iOp, iDest, src);
	}

	Instr *Instr::alterDest(Operand dest) {
		return new (this) Instr(iOp, dest, iSrc);
	}

	void Instr::toS(StrBuf *to) const {
		*to << code::name(iOp);

		if (iSrc.empty() && iDest.empty()) {
			// No operands.
		} else if (iDest.empty()) {
			*to << L" " << iSrc;
		} else if (iSrc.empty()) {
			*to << L" " << iDest;
		} else {
			*to << L" " << iDest << L", " << iSrc;
		}
	}

	Instr *instr(EnginePtr e, op::OpCode op) {
		return new (e.v) Instr(op, Operand(), Operand());
	}

	Instr *instrSrc(EnginePtr e, op::OpCode op, Operand src) {
		src.ensureReadable(op);
		return new (e.v) Instr(op, Operand(), src);
	}

	Instr *instrDest(EnginePtr e, op::OpCode op, Operand dest) {
		DestMode mode = destMode(op);
		if (mode == destNone)
			throw new (e.v) InvalidValue(S("Can not pass 'destNone' to 'instrDest'."));
		if (mode & destRead)
			dest.ensureReadable(op);
		if (mode & destWrite)
			dest.ensureWritable(op);
		return new (e.v) Instr(op, dest, Operand());
	}

	Instr *instrDestSrc(EnginePtr e, op::OpCode op, Operand dest, Operand src) {
		DestMode mode = destMode(op);
		if (mode == destNone)
			throw new (e.v) InvalidValue(S("Can not pass 'destNone' to 'instrDestSrc'."));
		if (mode & destRead)
			dest.ensureReadable(op);
		if (mode & destWrite)
			dest.ensureWritable(op);
		src.ensureReadable(op);
		if (dest.size() != src.size()) {
			Str *msg = TO_S(e.v, S("For ") << name(op)
							<< S(": Size of operands must match! ")
							<< dest << S(" vs ") << src);
			throw new (e.v) InvalidValue(msg);
		}
		return new (e.v) Instr(op, dest, src);
	}

	Instr *instrLoose(EnginePtr e, op::OpCode op, Operand dest, Operand src) {
		return new (e.v) Instr(op, dest, src);
	}

	/**
	 * Additional information, used with function calls.
	 */

	TypeInstr::TypeInstr(op::OpCode opCode, const Operand &dest, const Operand &src, TypeDesc *type, Bool member)
		: Instr(opCode, dest, src), type(type), member(member) {}

	void TypeInstr::deepCopy(CloneEnv *env) {
		Instr::deepCopy(env);
		cloned(type, env);
	}

	void TypeInstr::toS(StrBuf *to) const {
		Instr::toS(to);
		*to << S(" - ") << type;
		if (member)
			*to << S("[member]");
	}

	Instr *TypeInstr::alter(Operand dest, Operand src) {
		return new (this) TypeInstr(iOp, dest, src, type, member);
	}

	Instr *TypeInstr::alterSrc(Operand src) {
		return new (this) TypeInstr(iOp, iDest, src, type, member);
	}

	Instr *TypeInstr::alterDest(Operand dest) {
		return new (this) TypeInstr(iOp, dest, iSrc, type, member);
	}

	/**
	 * Instructions.
	 */

	static Operand sizedReg(Engine &e, Reg base, Size size) {
		Reg s = asSize(base, size);
		if (s == noReg) {
			if (size == Size())
				return Operand();
			else
				throw new (e) InvalidValue(S("The return size must fit in a register (ie. < 8 bytes)."));
		} else {
			return Operand(s);
		}
	}

	Instr *mov(EnginePtr e, Operand to, Operand from) {
		return instrDestSrc(e, op::mov, to, from);
	}

	Instr *swap(EnginePtr e, Reg a, Operand b) {
		b.ensureWritable(op::swap);
		return instrDestSrc(e, op::swap, a, b);
	}

	Instr *lea(EnginePtr e, Operand to, Operand from) {
		if (to.size() != Size::sPtr)
			throw new (e.v) InvalidValue(S("Lea must update a pointer."));

		switch (from.type()) {
		case opRelative:
		case opVariable:
		case opReference:
			// These are ok.
			break;
		default: {
			Str *msg = TO_S(e.v, S("lea must be used with a complex addressing mode or a reference.")
							S(" (Got ") << from << S(")"));
			throw new (e.v) InvalidValue(msg);
		}
		}
		return instrLoose(e, op::lea, to, from);
	}

	Instr *push(EnginePtr e, Operand v) {
		return instrSrc(e, op::push, v);
	}

	Instr *pop(EnginePtr e, Operand to) {
		return instrDest(e, op::pop, to);
	}

	Instr *pushFlags(EnginePtr e) {
		return instr(e, op::pushFlags);
	}

	Instr *popFlags(EnginePtr e) {
		return instr(e, op::popFlags);
	}

	Instr *jmp(EnginePtr e, Operand to) {
		if (to.size() != Size::sPtr)
			throw new (e.v) InvalidValue(TO_S(e.v, S("Must jump to a pointer, trying to jump to ") << to));
		return instrLoose(e, op::jmp, to, CondFlag(ifAlways));
	}

	Instr *jmp(EnginePtr e, Label to, CondFlag cond) {
		return instrLoose(e, op::jmp, to, cond);
	}

	Instr *setCond(EnginePtr e, Operand to, CondFlag cond) {
		if (to.size() != Size::sByte)
			throw new (e.v) InvalidValue(S("Must set a byte."));
		return instrLoose(e, op::setCond, to, cond);
	}

	Instr *call(EnginePtr e, Operand to, Size ret) {
		if (to.size() != Size::sPtr)
			throw new (e.v) InvalidValue(TO_S(e.v, S("Must call a pointer, tried calling ") << to));

		return instrLoose(e, op::call, sizedReg(e.v, ptrA, ret), to);
	}

	Instr *ret(EnginePtr e, Size ret) {
		Operand r = sizedReg(e.v, ptrA, ret);
		if (r.type() == opNone)
			return instr(e, op::ret);
		else
			return instrSrc(e, op::ret, r);
	}

	Instr *fnParam(EnginePtr e, TypeDesc *type, Operand src) {
		if (src.size() != type->size()) {
			Str *msg = TO_S(e.v, S("Size mismatch for 'fnParam'. Got ") << src.size() << S(", expected ") << type->size());
			throw new (e.v) InvalidValue(msg);
		}
		return new (e.v) TypeInstr(op::fnParam, Operand(), src, type, false);
	}

	Instr *fnParamRef(EnginePtr e, TypeDesc *type, Operand src) {
		if (src.size() != Size::sPtr)
			throw new (e.v) InvalidValue(TO_S(e.v, S("Must use a pointer with 'fnParamRef'. Used ") << src));
		return new (e.v) TypeInstr(op::fnParamRef, Operand(), src, type, false);
	}

	Instr *fnCall(EnginePtr e, Operand call, Bool member) {
		if (call.type() == opConstant)
			throw new (e.v) InvalidValue(S("Should not call constant values, use references instead!"));
		if (call.size() != Size::sPtr)
			throw new (e.v) InvalidValue(TO_S(e.v, S("Must call a pointer, tried calling ") << call));
		return new (e.v) TypeInstr(op::fnCall, Operand(), call, voidDesc(e), member);
	}

	Instr *fnCall(EnginePtr e, Operand call, Bool member, TypeDesc *result, Operand to) {
		if (call.type() == opConstant)
			throw new (e.v) InvalidValue(S("Should not call constant values, use references instead!"));
		if (call.size() != Size::sPtr)
			throw new (e.v) InvalidValue(TO_S(e.v, S("Must call a pointer, tried calling ") << call));
		if (to.size() != result->size()) {
			Str *msg = TO_S(e.v, S("Size mismatch for 'fnCall'. Got ") << to.size() << S(", expected ")  << result->size());
			throw new (e.v) InvalidValue(msg);
		}
		return new (e.v) TypeInstr(op::fnCall, to, call, result, member);
	}

	Instr *fnCallRef(EnginePtr e, Operand call, Bool member, TypeDesc *result, Operand to) {
		if (call.type() == opConstant)
			throw new (e.v) InvalidValue(S("Should not call constant values, use references instead!"));
		if (call.size() != Size::sPtr)
			throw new (e.v) InvalidValue(TO_S(e.v, S("Must call a pointer, tried calling ") << call));
		if (to.size() != Size::sPtr)
			throw new (e.v) InvalidValue(TO_S(e.v, S("Must use a pointer with 'fnCallRef'. Used ") << to));
		return new (e.v) TypeInstr(op::fnCallRef, to, call, result, member);
	}

	Instr *fnRet(EnginePtr e, Operand src) {
		return instrSrc(e, op::fnRet, src);
	}

	Instr *fnRetRef(EnginePtr e, Operand src) {
		if (src.size() != Size::sPtr)
			throw new (e.v) InvalidValue(TO_S(e.v, S("Must use a pointer with 'fnRetRef'. Used ") << src));
		return instrSrc(e, op::fnRetRef, src);
	}

	Instr *fnRet(EnginePtr e) {
		return instr(e, op::fnRet);
	}

	Instr *bor(EnginePtr e, Operand dest, Operand src) {
		return instrDestSrc(e, op::bor, dest, src);
	}

	Instr *band(EnginePtr e, Operand dest, Operand src) {
		return instrDestSrc(e, op::band, dest, src);
	}

	Instr *bxor(EnginePtr e, Operand dest, Operand src) {
		return instrDestSrc(e, op::bxor, dest, src);
	}

	Instr *bnot(EnginePtr e, Operand dest) {
		return instrDest(e, op::bnot, dest);
	}

	Instr *test(EnginePtr e, Operand dest, Operand src) {
		return instrDestSrc(e, op::test, dest, src);
	}

	Instr *add(EnginePtr e, Operand dest, Operand src) {
		return instrDestSrc(e, op::add, dest, src);
	}

	Instr *adc(EnginePtr e, Operand dest, Operand src) {
		return instrDestSrc(e, op::adc, dest, src);
	}

	Instr *sub(EnginePtr e, Operand dest, Operand src) {
		return instrDestSrc(e, op::sub, dest, src);
	}

	Instr *sbb(EnginePtr e, Operand dest, Operand src) {
		return instrDestSrc(e, op::sbb, dest, src);
	}

	Instr *cmp(EnginePtr e, Operand dest, Operand src) {
		return instrDestSrc(e, op::cmp, dest, src);
	}

	Instr *mul(EnginePtr e, Operand dest, Operand src) {
		return instrDestSrc(e, op::mul, dest, src);
	}

	Instr *idiv(EnginePtr e, Operand dest, Operand src) {
		return instrDestSrc(e, op::idiv, dest, src);
	}

	Instr *udiv(EnginePtr e, Operand dest, Operand src) {
		return instrDestSrc(e, op::udiv, dest, src);
	}

	Instr *imod(EnginePtr e, Operand dest, Operand src) {
		return instrDestSrc(e, op::imod, dest, src);
	}

	Instr *umod(EnginePtr e, Operand dest, Operand src) {
		return instrDestSrc(e, op::umod, dest, src);
	}

	Instr *shl(EnginePtr e, Operand dest, Operand src) {
		if (src.size() != Size::sByte)
			throw new (e.v) InvalidValue(S("Size for shl must be 1"));
		return instrLoose(e, op::shl, dest, src);
	}

	Instr *shr(EnginePtr e, Operand dest, Operand src) {
		if (src.size() != Size::sByte)
			throw new (e.v) InvalidValue(S("Size for shr must be 1"));
		return instrLoose(e, op::shr, dest, src);
	}

	Instr *sar(EnginePtr e, Operand dest, Operand src) {
		if (src.size() != Size::sByte)
			throw new (e.v) InvalidValue(S("Size for sar must be 1"));
		return instrLoose(e, op::sar, dest, src);
	}

	Instr *icast(EnginePtr e, Operand dest, Operand src) {
		return instrLoose(e, op::icast, dest, src);
	}

	Instr *ucast(EnginePtr e, Operand dest, Operand src) {
		return instrLoose(e, op::ucast, dest, src);
	}

	static void fpSize(Engine &e, Operand op) {
		Size sz = op.size();
		if (sz != Size::sFloat && sz != Size::sDouble)
			throw new (e) InvalidValue(S("Invalid size of floating point operand."));
	}

	Instr *fadd(EnginePtr e, Operand dest, Operand src) {
		fpSize(e.v, dest);
		return instrDestSrc(e, op::fadd, dest, src);
	}

	Instr *fsub(EnginePtr e, Operand dest, Operand src) {
		fpSize(e.v, dest);
		return instrDestSrc(e, op::fsub, dest, src);
	}

	Instr *fneg(EnginePtr e, Operand dest, Operand src) {
		fpSize(e.v, dest);
		return instrDestSrc(e, op::fneg, dest, src);
	}

	Instr *fmul(EnginePtr e, Operand dest, Operand src) {
		fpSize(e.v, dest);
		return instrDestSrc(e, op::fmul, dest, src);
	}

	Instr *fdiv(EnginePtr e, Operand dest, Operand src) {
		fpSize(e.v, dest);
		return instrDestSrc(e, op::fdiv, dest, src);
	}

	Instr *fcmp(EnginePtr e, Operand dest, Operand src) {
		fpSize(e.v, dest);
		return instrDestSrc(e, op::fcmp, dest, src);
	}

	Instr *fcast(EnginePtr e, Operand dest, Operand src) {
		fpSize(e.v, src);
		fpSize(e.v, dest);
		return instrLoose(e, op::fcast, dest, src);
	}

	Instr *fcasti(EnginePtr e, Operand dest, Operand src) {
		fpSize(e.v, src);
		return instrLoose(e, op::fcasti, dest, src);
	}

	Instr *fcastu(EnginePtr e, Operand dest, Operand src) {
		fpSize(e.v, src);
		return instrLoose(e, op::fcastu, dest, src);
	}

	Instr *icastf(EnginePtr e, Operand dest, Operand src) {
		fpSize(e.v, dest);
		return instrLoose(e, op::icastf, dest, src);
	}

	Instr *ucastf(EnginePtr e, Operand dest, Operand src) {
		fpSize(e.v, dest);
		return instrLoose(e, op::ucastf, dest, src);
	}

	Instr *fstp(EnginePtr e, Operand dest) {
		if (dest.type() == opRegister)
			throw new (e.v) InvalidValue(S("Can not store to register."));
		if (dest.size() != Size::sFloat && dest.size() != Size::sDouble)
			throw new (e.v) InvalidValue(S("Invalid size."));
		return instrDest(e, op::fstp, dest);
 	}

	Instr *fld(EnginePtr e, Operand src) {
		if (src.type() == opRegister)
			throw new (e.v) InvalidValue(S("Can not load a float from register."));
		if (src.type() == opConstant)
			throw new (e.v) InvalidValue(S("Can not load a float from a constant."));
		if (src.size() != Size::sFloat && src.size() != Size::sDouble)
			throw new (e.v) InvalidValue(S("Invalid size."));
		return instrSrc(e, op::fld, src);
	}

	Instr *dat(EnginePtr e, Operand v) {
		switch (v.type()) {
		case opConstant:
		case opLabel:
		case opReference:
		case opObjReference:
			break;
		default:
			throw new (e.v) InvalidValue(S("Cannot store other than references, constants and labels in dat"));
		}
		return instrSrc(e, op::dat, v);
	}

	Instr *lblOffset(EnginePtr e, Label l) {
		return instrSrc(e, op::lblOffset, l);
	}

	Instr *align(EnginePtr e, Offset o) {
		return instrSrc(e, op::align, ptrConst(o));
	}

	Instr *alignAs(EnginePtr e, Size a) {
		return align(e, Offset(a.align32(), a.align64()));
	}

	Instr *prolog(EnginePtr e) {
		return instr(e, op::prolog);
	}

	Instr *epilog(EnginePtr e) {
		return instr(e, op::epilog);
	}

	Instr *preserve(EnginePtr e, Operand dest, Reg reg) {
		return instrDestSrc(e, op::preserve, dest, reg);
	}

	Instr *location(EnginePtr e, SrcPos pos) {
		return instrLoose(e, op::location, Operand(), pos);
	}

	Instr *meta(EnginePtr e, Object *data) {
		return instrLoose(e, op::meta, Operand(), objPtr(data));
	}

	Instr *meta(EnginePtr e, TObject *data) {
		return instrLoose(e, op::meta, Operand(), objPtr(data));
	}

	Instr *begin(EnginePtr e, Block block) {
		return instrLoose(e, op::beginBlock, Operand(), block);
	}

	Instr *end(EnginePtr e, Block block) {
		return instrLoose(e, op::endBlock, Operand(), block);
	}

	Instr *jmpBlock(EnginePtr e, Label to, Block block) {
		return instrLoose(e, op::jmpBlock, to, block);
	}

	Instr *activate(EnginePtr e, Var var) {
		return instrLoose(e, op::activate, Operand(), var);
	}

	Instr *threadLocal(EnginePtr e) {
		return instr(e, op::threadLocal);
	}

}
