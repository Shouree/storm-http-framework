#include "stdafx.h"
#include "OpCode.h"
#include "OpTable.h"

using namespace code::op;

namespace code {

	struct OpInfo {
		const wchar *name;
		DestMode mode;
	};

#define W(x) S(x)

#define OP_CODE(x, mode) { op::x, { W(#x), mode } }
#define OP_ALT_CODE(x, mode, name) { op::x, { W(#name), mode } }

	static OpEntry<OpInfo> descs[] = {
		OP_CODE(nop, destNone),
		OP_CODE(mov, destWrite),
		OP_CODE(lea, destWrite),
		OP_CODE(push, destNone),
		OP_CODE(pop, destWrite),
		OP_CODE(pushFlags, destNone),
		OP_CODE(popFlags, destNone),
		OP_CODE(jmp, destRead),
		OP_CODE(call, destRead),
		OP_CODE(ret, destNone),
		OP_CODE(setCond, destWrite),
		OP_CODE(fnParam, destNone),
		OP_CODE(fnParamRef, destNone),
		OP_CODE(fnCall, destRead),
		OP_CODE(fnCallRef, destRead),
		OP_CODE(fnRet, destNone),
		OP_CODE(fnRetRef, destNone),
		OP_CODE(swap, destRead | destWrite),
		OP_ALT_CODE(bor, destRead | destWrite, or),
		OP_ALT_CODE(band, destRead | destWrite, and),
		OP_ALT_CODE(bxor, destRead | destWrite, xor),
		OP_ALT_CODE(bnot, destRead | destWrite, not),
		OP_CODE(test, destRead),
		OP_CODE(add, destRead | destWrite),
		OP_CODE(adc, destRead | destWrite),
		OP_CODE(sub, destRead | destWrite),
		OP_CODE(sbb, destRead | destWrite),
		OP_CODE(cmp, destRead),
		OP_CODE(mul, destRead | destWrite),
		OP_CODE(idiv, destRead | destWrite),
		OP_CODE(udiv, destRead | destWrite),
		OP_CODE(imod, destRead | destWrite),
		OP_CODE(umod, destRead | destWrite),
		OP_CODE(shl, destRead | destWrite),
		OP_CODE(shr, destRead | destWrite),
		OP_CODE(sar, destRead | destWrite),
		OP_CODE(icast, destWrite),
		OP_CODE(ucast, destWrite),
		OP_CODE(fcast, destWrite),
		OP_CODE(fadd, destRead | destWrite),
		OP_CODE(fsub, destRead | destWrite),
		OP_CODE(fneg, destWrite),
		OP_CODE(fmul, destRead | destWrite),
		OP_CODE(fdiv, destRead | destWrite),
		OP_CODE(fcmp, destRead),
		OP_CODE(fcasti, destWrite),
		OP_CODE(fcastu, destWrite),
		OP_CODE(icastf, destWrite),
		OP_CODE(ucastf, destWrite),
		OP_CODE(fstp, destWrite),
		OP_CODE(fld, destNone),
		OP_CODE(dat, destNone),
		OP_CODE(lblOffset, destNone),
		OP_CODE(align, destNone),
		OP_CODE(prolog, destNone),
		OP_CODE(epilog, destNone),
		OP_CODE(preserve, destWrite),
		OP_CODE(location, destNone),
		OP_CODE(meta, destNone),
		OP_ALT_CODE(beginBlock, destNone, begin),
		OP_ALT_CODE(endBlock, destNone, end),
		OP_CODE(jmpBlock, destNone),
		OP_CODE(activate, destNone),
		OP_CODE(threadLocal, destNone),
	};

	static const OpInfo &info(op::OpCode op) {
		static OpTable<OpInfo> ops(descs, ARRAY_COUNT(descs));
		return ops[op];
	}

	const wchar *name(op::OpCode op) {
		return info(op).name;
	}

	DestMode destMode(op::OpCode op) {
		return info(op).mode;
	}

}
