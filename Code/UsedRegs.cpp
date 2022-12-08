#include "stdafx.h"
#include "UsedRegs.h"
#include "Arena.h"

namespace code {

	using impl::WorkItem;

	impl::WorkItem::WorkItem(Nat line) : line(line), inWork(false), nextDep(null), nextWork(null) {}

	impl::WorkItem::WorkItem(Nat line, WorkItem *nextDep) : line(line), inWork(false), nextDep(nextDep), nextWork(null) {}


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

	class RegState {
	public:
		RegState(const Arena *arena, const Listing *src)
			: arena(arena), src(src), work(null), workEnd(null) {

			Engine &e = src->engine();
			usedNow = new (e) RegSet();
			used = new (e) Array<RegSet *>(src->count(), null);
			for (Nat i = 0; i < src->count(); i++)
				used->at(i) = new (e) RegSet();

			labelSrc = runtime::allocArray<Nat>(e, &natArrayType, src->labelCount());
			labelDeps = runtime::allocArray<WorkItem *>(e, StormInfo<WorkItem *>::handle(e).gcArrayType, src->labelCount());
		}


		// Arena.
		const Arena *arena;

		// Listing.
		const Listing *src;

		// Variable for 'usedNow', so we don't have to re-allocate it many times.
		RegSet *usedNow;

		// Set of registers used for all lines in the code.
		Array<RegSet *> *used;

		// Source line for labels.
		GcArray<Nat> *labelSrc;

		// Dependencies for all labels encountered so far.
		GcArray<WorkItem *> *labelDeps;

		// List of work to perform.
		WorkItem *work;

		// End of the list of work.
		WorkItem *workEnd;

		// Add work to the work-list.
		void addWork(WorkItem *item) {
			if (!item)
				return;

			// Add them in reverse order to make sure the highest line is processed first.
			// This list will never be very long, so a recursive add is fine.
			if (item->nextDep)
				addWork(item->nextDep);

			if (item->inWork)
				return;

			item->inWork = true;
			if (workEnd) {
				workEnd->nextWork = item;
				workEnd = item;
			} else {
				work = workEnd = item;
			}
		}

		// Pop an item from the work queue.
		WorkItem *popWork() {
			WorkItem *result = work;
			if (result)
				work = result->nextWork;
			if (!work)
				workEnd = null;
			return result;
		}
	};

	static Bool isLabelJump(Instr *instr) {
		return (instr->op() == op::jmp) & (instr->dest().type() == opLabel);
	}

	static Bool processJump(RegState &state, Instr *instr, Nat target, RegSet *usedNow) {
		Nat targetLine = state.labelSrc->v[target];
		switch (instr->src().condFlag()) {
		case ifAlways:
			// Only use the target.
			usedNow->set(state.used->at(targetLine));
			return true;
		case ifNever:
			// Nothing special to do.
			return false;
		default:
			usedNow->put(state.used->at(targetLine));
			return true;
		}
	}

	// First-time traversal. Always traverses all instructions in the listing.
	static void traverseFirst(RegState &state, RegSet *all) {
		RegSet *usedNow = state.usedNow;
		usedNow->clear();

		for (Nat i = state.src->count(); i > 0; i--) {
			Nat line = i - 1;
			Instr *instr = state.src->at(line);

			if (isLabelJump(instr)) {
				Nat target = instr->dest().label().key();
				if (processJump(state, instr, target, usedNow)) {
					// Record the dependency
					state.labelDeps->v[target] = new (state.src) WorkItem(line, state.labelDeps->v[target]);
				}
			} else {
				processInstruction(state.arena, instr, usedNow);
			}

			state.used->at(line)->set(usedNow);

			// Keep 'all' up to date.
			if (instr->mode() & destWrite)
				*all += instr->dest();

			// If there are labels, we always post them to the work-list. Since we work backwards,
			// this means that only back-edges will be added to the work-list, which happens to be
			// exactly what we want.
			// Also: record line numbers for labels so that we can use them in 'processJump'.
			if (Array<Label> *labels = state.src->labels(line)) {
				for (Nat i = 0; i < labels->count(); i++) {
					Nat lbl = labels->at(i).key();
					state.labelSrc->v[lbl] = line;
					state.addWork(state.labelDeps->v[lbl]);
				}
			}
		}
	}

	// Subsequent traversals. Processes rows from the starting line until the state no longer
	// changes. Triggers items on the work-list as needed.
	// Note: if we happen to process "too far" on one iteration, any remaining work items
	// that we happened to process "early" will simply terminate almost immediately when
	// they are processed.
	static void traverseNext(RegState &state, Nat start) {
		RegSet *usedNow = state.usedNow;
		if (start + 1 < state.src->count())
			usedNow->set(state.used->at(start + 1));
		else
			usedNow->clear();

		for (Nat i = start + 1; i > 0; i--) {
			Nat line = i - 1;
			Instr *instr = state.src->at(line);

			if (isLabelJump(instr)) {
				processJump(state, instr, instr->dest().label().key(), usedNow);

				// TODO: With slightly better data structures we could remove this node from the
				// work-list at this point if it is there. This means that we will avoid some calls
				// to the 'traverseNext' function that would be unneccessary. Our check below would
				// still make it terminate quickly, so it is not too expensive.
			} else {
				processInstruction(state.arena, instr, usedNow);
			}

			Bool changed = *usedNow != *state.used->at(line);
			// Since we have our work-list, we can just exit whenever the state stops changing.
			if (!changed)
				return;

			state.used->at(line)->set(usedNow);

			// Post new items to the work queue.
			if (Array<Label> *labels = state.src->labels(line)) {
				for (Nat i = 0; i < labels->count(); i++) {
					Nat lbl = labels->at(i).key();
					state.addWork(state.labelDeps->v[lbl]);
				}
			}
		}
	}

	UsedRegs usedRegs(const Arena *arena, const Listing *src) {
		// This function implements a backwards-flow fixpoint algorithm for finding an
		// over-estimation of the used registers at the point of each instruction.
		// This is done by iterating through the instructions backwards in the code
		// and computing all usages. Any back-edges are detected, and re-computed at
		// a later pass. This might need to be repeated a number of times until the
		// computation converges.

		RegState state(arena, src);

		// All registers.
		RegSet *all = new (src) RegSet();

		// First pass. Simply go through the code from the end and update the state as necessary.
		traverseFirst(state, all);

		// Process items on the work-list as necessary.
		while (WorkItem *work = state.popWork()) {
			traverseNext(state, work->line);
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
