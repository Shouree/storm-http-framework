#include "stdafx.h"
#include "UsedRegs.h"
#include "Arena.h"

namespace code {

	UsedRegs::UsedRegs(Array<RegSet *> *used, RegSet *all) : used(used), all(all) {}

	static void operator +=(RegSet &to, const Operand &op) {
		if (op.type() == opRegister)
			to.put(op.reg());
	}

	static void operator -=(RegSet &to, const Operand &op) {
		if (op.type() == opRegister)
			to.remove(op.reg());
	}

	static void addIndirect(RegSet *to, const Operand &op) {
		if (op.type() == opRelative)
			to->put(op.reg());
	}

	static void addIndirect(RegSet *to, Instr *instr) {
		addIndirect(to, instr->src());
		addIndirect(to, instr->dest());
	}

	static void processInstr(const Arena *arena, Instr *instr, RegSet *all, RegSet *used) {
		switch (instr->op()) {
		case op::endBlock:
		case op::jmpBlock:
		case op::prolog:
			// These do not preserve any registers.
			used->clear();
			break;
		case op::jmp:
			// If it is an unconditional jump, don't preserve registers. Otherwise, might preserve.
			if (instr->src().condFlag() == ifAlways)
				used->clear();
			break;
		case op::beginBlock:
			// Does not preserve 'rax'.
			used->remove(ptrA);
			break;
		case op::swap:
			// No effect on the usage.
			break;
		case op::fnCall:
		case op::fnCallRef:
		case op::call:
			// Not all registers preserved.
			if (arena)
				arena->removeFnRegs(used);
			else
				used->clear();
			// Intentional fall-through.
		default:
			if (instr->op() == op::bxor &&
				instr->src() == instr->dest()) {

				// We're just setting 'src' to 0.
				used->clear();
				break;
			}

			addIndirect(used, instr);
			*used += instr->src();

			if (instr->mode() & destWrite) {
				*all += instr->dest();
				*used -= instr->dest();
			}

			if (instr->mode() & destRead) {
				*used += instr->dest();
			}

			break;
		}
	}

	UsedRegs usedRegs(const Arena *arena, const Listing *src) {
		TODO(L"Fix the implementation!");

		Array<RegSet *> *used = new (src) Array<RegSet *>(src->count(), null);

		RegSet *all = new (src) RegSet();
		RegSet *usedNow = new (src) RegSet();

		for (Nat i = src->count(); i > 0; i--) {
			Instr *instr = src->at(i - 1);

			processInstr(arena, instr, all, usedNow);

			used->at(i - 1) = new (src) RegSet(*usedNow);
		}


		return UsedRegs(used, all);
	}

	RegSet *allUsedRegs(const Listing *src) {
		RegSet *all = new (src) RegSet();

		for (Nat i = 0; i < src->count(); i++) {
			Instr *instr = src->at(i);

			if (instr->mode() & destWrite)
				*all += instr->dest();
		}

		return all;
	}

}
