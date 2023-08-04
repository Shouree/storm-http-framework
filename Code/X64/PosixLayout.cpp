#include "stdafx.h"
#include "PosixLayout.h"
#include "Code/Layout.h"

namespace code {
	namespace x64 {

		PosixLayout::PosixLayout(const Arena *arena) : Layout(arena) {}

		Offset PosixLayout::resultParam() {
			Nat count = 1 + toPreserve->count();
			return -(Offset::sPtr * count);
		}

		void PosixLayout::saveResult(Listing *dest) {
			const Result &result = params->result();

			if (result.memoryRegister() != noReg) {
				// We need to save ptrA that contains the return address. Twice to keep the stack aligned.
				*dest << push(ptrA);
				*dest << push(ptrA);
			} else if (result.registerCount() > 0) {
				Size sz = Size::sPtr * roundUp(result.registerCount(), Nat(2));
				*dest << sub(ptrStack, ptrConst(sz));
				for (Nat i = 0; i < result.registerCount(); i++) {
					*dest << mov(ptrRel(ptrStack, Offset::sPtr * i), asSize(result.registerAt(i), Size::sPtr));
				}
			}
		}

		void PosixLayout::restoreResult(Listing *dest) {
			const Result &result = params->result();

			if (result.memoryRegister() != noReg) {
				*dest << pop(ptrA);
				*dest << pop(ptrA);
			} else if (result.registerCount() > 0) {
				Size sz = Size::sPtr * roundUp(result.registerCount(), Nat(2));
				for (Nat i = 0; i < result.registerCount(); i++) {
					*dest << mov(asSize(result.registerAt(i), Size::sPtr), ptrRel(ptrStack, Offset::sPtr * i));
				}
				*dest << add(ptrStack, ptrConst(sz));
			}
		}

		void PosixLayout::emitProlog(Listing *dest) {
			*dest << prolog();

			// Allocate stack space.
			if (layout->last() != Offset())
				*dest << sub(ptrStack, ptrConst(layout->last()));

			// Keep track of offsets.
			Offset offset = -Offset::sPtr;

			// Save registers we need to preserve.
			for (RegSet::Iter i = toPreserve->begin(); i != toPreserve->end(); ++i) {
				*dest << mov(ptrRel(ptrFrame, offset), asSize(i.v(), Size::sPtr));
				*dest << preserve(ptrRel(ptrFrame, offset), asSize(i.v(), Size::sPtr));
				offset -= Offset::sPtr;
			}

			// Spill parameters to the stack.
			spillParams(dest);
		}

		void PosixLayout::emitEpilog(Listing *dest) {
			// Restore preserved registers.
			Offset offset = -Offset::sPtr;
			for (RegSet::Iter i = toPreserve->begin(); i != toPreserve->end(); ++i) {
				*dest << mov(asSize(i.v(), Size::sPtr), ptrRel(ptrFrame, offset));
				offset -= Offset::sPtr;
			}

			// The "epilog" pseudo-op generates a LEAVE instruction, which corresponds to these instructions:
			// *dest << mov(ptrStack, ptrFrame);
			// *dest << pop(ptrFrame);

			*dest << code::epilog();
		}

		static Size spillParams(Array<Offset> *out, Listing *l, Params *params, Offset varOffset) {
			// NOTE: We could avoid spilling primitives to memory, as we do not generally use those
			// registers from code generated for the generic platform. However, we would need to
			// save as soon as we perform a function call anyway, which is probably more expensive
			// than just spilling them unconditionally in the function prolog.

			Array<Var> *all = l->allParams();

			{
				// Start by computing the locations of parameters passed on the stack.
				Offset stackOffset = Offset::sPtr * 2; // return addr, rbp
				for (Nat i = 0; i < params->stackCount(); i++) {
					Nat paramId = params->stackParam(i).id(); // Should never be 'Param::returnId'
					Var var = all->at(paramId);

					out->at(var.key()) = stackOffset + Offset(params->stackOffset(i));
				}
			}

			Size used;
			// Then, compute where to spill the remaining registers. Keep track if they are passed
			// in memory or not, since that affects their size!
			for (Nat i = 0; i < params->registerCount(); i++) {
				Param p = params->registerParam(i);
				if (p.empty())
					continue;
				if (p.id() == Param::returnId())
					continue;

				Var var = all->at(p.id());
				Offset &to = out->at(var.key());

				// Already computed -> no need to spill.
				if (to != Offset())
					continue;

				// Try to squeeze the preserved registers as tightly as possible while keeping alignment.
				// This is more concentrated than when pushing registers to the stack. On the stack, all
				// parameters are aligned to 8 bytes, while we do not need that here. We only need to keep
				// the natural alignment of the parameters to keep the CPU happy.
				Size sz = p.size();
				if (l->freeOpt(var) & freeIndirection)
					sz = Size::sPtr;

				// However, since we don't want to use multiple instructions to spill a single
				// parameter, we increase the size of some parameters a bit.
				if (asSize(ptrA, sz) == noReg)
					sz = sz + Size::sInt.alignment();

				used = (used + sz).alignedAs(sz);
				to = -(varOffset + used);
			}

			return used.aligned();
		}

		Array<Offset> *PosixLayout::computeLayout(Listing *l, Params *params, Nat spilled) {
			Array<Offset> *result = code::layout(l);

			// Check if we need to spill the result parameter as well!
			if (params->result().memoryRegister() != noReg)
				spilled++;

			// Saved registers and other things:
			Offset varOffset = Offset::sPtr * spilled;

			// Figure out which parameters we need to spill into memory.
			varOffset += code::x64::spillParams(result, l, params, varOffset);
			varOffset = varOffset.alignAs(Size::sPtr);

			// Update the layout of the other variables according to the size we needed for
			// parameters and spilled registers.
			Array<Var> *all = l->allVars();
			for (Nat i = 0; i < all->count(); i++) {
				Var var = all->at(i);
				Nat id = var.key();

				if (!l->isParam(var)) {
					Size s = var.size();
					if (l->freeOpt(var) & freeIndirection)
						s = Size::sPtr;
					result->at(id) = -(result->at(id) + s.aligned() + varOffset);
				}
			}

			result->last() += varOffset;
			if (result->last().v64() & 0xF) {
				// Need to be aligned to 64 bits. Otherwise, the SIMD operations used widely on
				// X86-64 will not work properly.
				result->last() = Offset(roundUp(result->last().v64(), 0x10));
				// result->last() += Offset::sPtr;
			}

			return result;
		}

	}
}
