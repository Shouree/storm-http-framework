#pragma once
#include "Utils/Bitmask.h"

namespace code {

	/**
	 * Declare all virtual op-codes.
	 */
	namespace op {
		STORM_PKG(core.asm);

		enum OpCode {
			nop,
			mov,
			lea,
			push,
			pop,
			pushFlags,
			popFlags,
			jmp,
			call,
			ret,
			setCond,
			fnParam,
			fnParamRef,
			fnCall,
			fnCallRef,
			fnRet,
			fnRetRef,
			STORM_NAME(bor, or),
			STORM_NAME(band, and),
			STORM_NAME(bxor, xor),
			STORM_NAME(bnot, not),
			add,
			adc,
			sub,
			sbb,
			cmp,
			test,
			mul,
			idiv,
			udiv,
			imod,
			umod,
			shl,
			shr,
			sar,
			swap,
			icast,
			ucast,

			// Floating point.
			fadd,
			fsub,
			fneg,
			fmul,
			fdiv,
			fcmp,
			ftoi,
			ftou,
			fcast,  // float -> float
			fcasti, // float -> int
			fcastu, // float -> unsigned
			icastf, // int -> float
			ucastf, // unsigned -> float

			// FP stack (to support calling convention on x86 in 32-bit mode).
			fstp,
			fld,

			// Data
			dat,
			lblOffset,
			align,

			// Function prolog/epilog.
			prolog,
			epilog,

			// Note that a register has been preserved somewhere. Used to generate debugging information.
			preserve,

			// Source code reference, used for debug information or other transformations.
			location,

			// Other metadata. Does not emit any machine code.
			meta,

			// Blocks.
			beginBlock,
			endBlock,

			// Jump to a location making sure to go to a particular block.
			jmpBlock,

			// Variable activation.
			activate,

			// Make the next memory operation work on the thread local storage.
			threadLocal,

			// Keep last
			numOpCodes,
		};
	}

	/**
	 * What is done to the 'dest' of the instructions?
	 */
	enum DestMode {
		STORM_NAME(destNone, none) = 0x0,
		STORM_NAME(destRead, read) = 0x1,
		STORM_NAME(destWrite, write) = 0x2,
	};

	BITMASK_OPERATORS(DestMode);

	const wchar *name(op::OpCode op);
	DestMode destMode(op::OpCode op);
}
