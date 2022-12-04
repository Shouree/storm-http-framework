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

	static void processInstruction(const Arena *arena, Instr *instr, RegSet *used) {
		switch (instr->op()) {
		case op::endBlock:
		case op::jmpBlock:
		case op::prolog:
			// These do not preserve any registers. This means that 'used' should be empty by here
			// for "proper" programs.
			used->clear();
			break;
		case op::jmp:
			// Jumps to arbitrary points are handled by assuming no dependencies. Currently, they
			// are only used to jump to other functions, and other low-level things.
			used->clear();
			break;
		case op::beginBlock:
		case op::swap:
			// No effect on used registers;
			break;
		case op::fnCall:
		case op::fnCallRef:
		case op::call:
			// Only the registers preserved through fn calls can be assumed to be preserved.
			if (arena)
				arena->removeFnRegs(used);
			else
				used->clear();
			break;

		case op::bxor:
			if (instr->src() == instr->dest()) {
				// Used to zero registers.
				*used -= instr->src();
				break;
			}
			// Intentional fall-through.
		default:
			addIndirect(used, instr);
			*used += instr->src();
			if (instr->mode() & destWrite)
				*used -= instr->dest();
			if (instr->mode() & destRead)
				*used += instr->dest();
			break;
		}
	}

	struct RegState {
		// Arena.
		const Arena *arena;

		// Listing.
		const Listing *src;

		// Set of registers used for all lines in the code.
		Array<RegSet *> *used;

		// Location of all labels encountered so far.
		Array<Nat> *labelSrc;
	};

	// If 'all' is true, then we assume that this is the first pass and collect all used
	// registers. Otherwise, we assume it is a subsequent pass, where we don't need to update
	// 'labelSrc' either.
	// Returns true if we need more traversals.
	static Bool traverse(RegState &state, RegSet *all) {
		RegSet *usedNow = new (state.src) RegSet();
		Bool more = false;

		for (Nat i = state.src->count(); i > 0; i--) {
			Nat id = i - 1;
			Instr *instr = state.src->at(id);

			// Handle this instruction, save the result.
			if (instr->op() == op::jmp && instr->dest().type() == opLabel) {
				// For a jump, we should set 'usedNow' to the 'usedNow' of the target
				// instruction of the jump. If it is conditional, then we set it to the union of
				// what we have now and the target of the label.
				Nat target = instr->dest().label().key();
				Nat targetLine = state.labelSrc->at(target);
				if (instr->src().condFlag() == ifAlways) {
					// Unconditional: always pick target line's used.
					usedNow->set(state.used->at(targetLine));
				} else if (instr->src().condFlag() != ifNever) {
					// Either, make it the union.
					usedNow->put(state.used->at(targetLine));
				}

				if (targetLine < i) {
					// This involves a back-edge, we might need another pass (always if this is the
					// first time).
					if (all || *usedNow != *state.used->at(id))
						more = true;
				}

			} else {
				processInstruction(state.arena, instr, usedNow);
			}

			state.used->at(id)->set(usedNow);

			if (all) {
				// Keep 'all' up to date.
				if (instr->mode() & destWrite)
					*all += instr->dest();

				// Add labels, if any.
				if (Array<Label> *lbls = state.src->labels(id)) {
					for (Nat i = 0; i < lbls->count(); i++) {
						state.labelSrc->at(lbls->at(i).key()) = id;
					}
				}
			}
		}

		return more;
	}

	UsedRegs usedRegs(const Arena *arena, const Listing *src) {
		// This function implements a backwards-flow fixpoint algorithm for finding an
		// over-estimation of the used registers at the point of each instruction.
		// This is done by iterating through the instructions backwards in the code
		// and computing all usages. Any back-edges are detected, and re-computed at
		// a later pass. This might need to be repeated a number of times until the
		// computation converges.

		RegState state = {
			arena,
			src,
			new (src) Array<RegSet *>(src->count(), null),
			new (src) Array<Nat>(src->labelCount(), 0)
		};

		// All registers?
		RegSet *all = new (src) RegSet();

		// Fill 'used'.
		for (Nat i = 0; i < state.used->count(); i++)
			state.used->at(i) = new (src) RegSet();

		// First pass. Simply go through the code from the end and update the state as necessary.
		if (traverse(state, all)) {
			// Then, traverse until no more changes.
			TODO(L"This can be done in a more efficient manner: we don't need to traverse the entire thing.");
			while (traverse(state, null))
				;
		}

		return UsedRegs(state.used, all);
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
