#include "stdafx.h"
#include "Remove64.h"
#include "Asm.h"

namespace code {
	namespace x86 {

#define TRANSFORM(x) { op::x, &Remove64::x ## Tfm }

		const OpEntry<Remove64::TransformFn> Remove64::transformMap[] = {
			TRANSFORM(mov),
			TRANSFORM(swap),
			TRANSFORM(add),
			TRANSFORM(adc),
			TRANSFORM(bor),
			TRANSFORM(band),
			TRANSFORM(bnot),
			TRANSFORM(sub),
			TRANSFORM(sbb),
			TRANSFORM(bxor),
			TRANSFORM(cmp),
			TRANSFORM(test),
			TRANSFORM(mul),
			TRANSFORM(idiv),
			TRANSFORM(udiv),
			TRANSFORM(imod),
			TRANSFORM(umod),
			TRANSFORM(push),
			TRANSFORM(pop),
		};

		Remove64::Remove64() {}

		void Remove64::before(Listing *dest, Listing *src) {
			used = code::usedRegs(dest->arena, src).used;
		}

		void Remove64::during(Listing *dest, Listing *src, Nat line) {
			static OpTable<TransformFn> t(transformMap, ARRAY_COUNT(transformMap));

			Instr *i = src->at(line);
			TransformFn f = t[i->op()];
			if (i->size() == Size::sLong && f != null) {
				(this->*f)(dest, i, used->at(line));
			} else {
				*dest << i;
			}
		}

		// Call a function which implements the instruction. (reads src and dest, stores in dest).
		static void callFn(Listing *to, Instr *instr, RegSet *used, const void *fn) {
			Operand src = instr->src();
			Operand dest = instr->dest();

			used = new (used) RegSet(*used);

			// If we're supposed to clobber a register, don't try to preserve it!
			if (dest.type() == opRegister)
				used->remove(dest.reg());

			// Save registers
			Preserve preserve(fnDirtyRegs, fnDirtyCount, used, to);

			*to << push(high32(src));
			*to << push(low32(src));
			*to << push(high32(dest));
			*to << push(low32(dest));
			*to << call(xConst(Size::sPtr, (Word)fn), Size::sLong);
			*to << add(ptrStack, ptrConst(Size::sLong * 2));

			// Save the result somewhere
			if (dest != Operand(rax)) {

				// Need to consider that 'dest' might depend on something that we preserved
				// previously. ptrC is always free as it is clobbered by the function call.
				dest = preserve.updateRelative(dest, ptrC);

				*to << mov(high32(dest), high32(rax));
				*to << mov(low32(dest), low32(rax));
			}

			// Restore registers
			preserve.restore();
		}

		void Remove64::movTfm(Listing *to, Instr *instr, RegSet *used) {
			*to << mov(low32(instr->dest()), low32(instr->src()));
			*to << mov(high32(instr->dest()), high32(instr->src()));
		}

		void Remove64::swapTfm(Listing *to, Instr *instr, RegSet *used) {
			*to << swap(low32(instr->dest().reg()), low32(instr->src()));
			*to << swap(high32(instr->dest().reg()), high32(instr->src()));
		}

		void Remove64::addTfm(Listing *to, Instr *instr, RegSet *used) {
			*to << add(low32(instr->dest()), low32(instr->src()));
			*to << adc(high32(instr->dest()), high32(instr->src()));
		}

		void Remove64::adcTfm(Listing *to, Instr *instr, RegSet *used) {
			*to << adc(low32(instr->dest()), low32(instr->src()));
			*to << adc(high32(instr->dest()), high32(instr->src()));
		}

		void Remove64::borTfm(Listing *to, Instr *instr, RegSet *used) {
			*to << bor(low32(instr->dest()), low32(instr->src()));
			*to << bor(high32(instr->dest()), high32(instr->src()));
		}

		void Remove64::bandTfm(Listing *to, Instr *instr, RegSet *used) {
			*to << band(low32(instr->dest()), low32(instr->src()));
			*to << band(high32(instr->dest()), high32(instr->src()));
		}

		void Remove64::bnotTfm(Listing *to, Instr *instr, RegSet *used) {
			*to << bnot(low32(instr->dest()));
			*to << bnot(high32(instr->dest()));
		}

		void Remove64::subTfm(Listing *to, Instr *instr, RegSet *used) {
			*to << sub(low32(instr->dest()), low32(instr->src()));
			*to << sbb(high32(instr->dest()), high32(instr->src()));
		}

		void Remove64::sbbTfm(Listing *to, Instr *instr, RegSet *used) {
			*to << sbb(low32(instr->dest()), low32(instr->src()));
			*to << sbb(high32(instr->dest()), high32(instr->src()));
		}

		void Remove64::bxorTfm(Listing *to, Instr *instr, RegSet *used) {
			*to << bxor(low32(instr->dest()), low32(instr->src()));
			*to << bxor(high32(instr->dest()), high32(instr->src()));
		}

		void Remove64::cmpTfm(Listing *to, Instr *instr, RegSet *used) {
			Reg dest = unusedReg(used);

			bool preserved = false;
			if (dest == noReg) {
				*to << push(ebx);
				dest = ebx;
				preserved = true;
			} else {
				dest = asSize(dest, Size::sInt);
			}

			*to << mov(dest, low32(instr->dest()));
			*to << sub(dest, low32(instr->src()));
			*to << pushFlags();
			*to << mov(dest, high32(instr->dest()));
			*to << sbb(dest, high32(instr->src()));
			*to << pushFlags();

			// Reset ZF if it was not set the first time around.
			*to << mov(dest, intRel(ptrStack, Offset::sInt));
			*to << bor(dest, intConst(~(1 << 6))); // Now all bits except for ZF are always 1
			*to << band(intRel(ptrStack, Offset()), dest); // Masking out ZF.

			*to << popFlags();
			*to << pop(dest); // We can not use add here, as it modifies flags.

			if (preserved) {
				*to << pop(ebx);
			}

			// Wrong:
			// Label end = to->label();
			// *to << cmp(high32(instr->dest()), high32(instr->src()));
			// *to << jmp(end, ifNotEqual);
			// *to << cmp(low32(instr->dest()), low32(instr->src()));
			// *to << end;
		}

		void Remove64::testTfm(Listing *to, Instr *instr, RegSet *used) {
			// We only care about SF and ZF (sign and zero) here.

			Reg dest = unusedReg(used);

			bool preserved = false;
			if (dest == noReg) {
				*to << push(ebx);
				dest = ebx;
				preserved = true;
			} else {
				dest = asSize(dest, Size::sInt);
			}

			*to << mov(dest, low32(instr->dest()));
			*to << test(dest, low32(instr->src()));
			*to << pushFlags();
			*to << mov(dest, high32(instr->dest()));
			*to << test(dest, high32(instr->src()));
			*to << pushFlags();

			// Clear ZF if it was not set both times.
			*to << mov(dest, intRel(ptrStack, Offset::sInt));
			*to << bor(dest, intConst(~(1 << 6)));
			*to << band(intRel(ptrStack, Offset()), dest);

			*to << popFlags();
			*to << pop(dest); // We can not use add here, as it modifies flags.

			if (preserved) {
				*to << pop(ebx);
			}

		}

		static Long CODECALL mul(Long a, Long b) {
			return a * b;
		}

		void Remove64::mulTfm(Listing *to, Instr *instr, RegSet *used) {
			callFn(to, instr, used, address(&mul));
		}

		static Long CODECALL idiv(Long a, Long b) {
			return a / b;
		}

		void Remove64::idivTfm(Listing *to, Instr *instr, RegSet *used) {
			callFn(to, instr, used, address(&idiv));
		}

		static Word CODECALL udiv(Word a, Word b) {
			return a / b;
		}

		void Remove64::udivTfm(Listing *to, Instr *instr, RegSet *used) {
			callFn(to, instr, used, address(&udiv));
		}

		static Long CODECALL imod(Long a, Long b) {
			return a % b;
		}

		void Remove64::imodTfm(Listing *to, Instr *instr, RegSet *used) {
			callFn(to, instr, used, address(&imod));
		}

		static Word CODECALL umod(Word a, Word b) {
			return a % b;
		}

		void Remove64::umodTfm(Listing *to, Instr *instr, RegSet *used) {
			callFn(to, instr, used, address(&umod));
		}

		void Remove64::pushTfm(Listing *to, Instr *instr, RegSet *used) {
			*to << push(high32(instr->src()));
			*to << push(low32(instr->src()));
		}

		void Remove64::popTfm(Listing *to, Instr *instr, RegSet *used) {
			*to << pop(low32(instr->dest()));
			*to << pop(high32(instr->dest()));
		}

	}
}
