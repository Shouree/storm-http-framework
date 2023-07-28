#include "stdafx.h"
#include "WindowsLayout.h"
#include "Code/Layout.h"

namespace code {
	namespace x64 {

		WindowsLayout::WindowsLayout(const Arena *arena) : Layout(arena) {}

		Offset WindowsLayout::resultParam() {
			// Shadow space for rcx.
			return Offset::sPtr * 2;
		}

		void WindowsLayout::saveResult(Listing *dest) {
			const Result &result = params->result();

			// We have reserved space to store these right above the shadow space!
			for (Nat i = 0; i < result.registerCount(); i++) {
				*dest << mov(ptrRel(ptrStack, Offset::sPtr * Int(i + 4)), asSize(result.registerAt(i), Size::sPtr));
			}
		}

		void WindowsLayout::restoreResult(Listing *dest) {
			const Result &result = params->result();

			// We have reserved space to store these right above the shadow space!
			for (Nat i = 0; i < result.registerCount(); i++) {
				*dest << mov(asSize(result.registerAt(i), Size::sPtr), ptrRel(ptrStack, Offset::sPtr * Int(i + 4)));
			}
		}

		static bool spillShadow(const Param &p, const Offset &o) {
			(void)p;
			// Shadow space is at positive offsets from the frame pointer:
			return o.v64() > 0;
		}

		static bool spillRemaining(const Param &p, const Offset &o) {
			return !spillShadow(p, o);
		}

		void WindowsLayout::emitProlog(Listing *dest) {
			// Set up stack frame:
			*dest << push(ptrFrame);
			*dest << preserve(ptrFrame);
			*dest << mov(ptrFrame, ptrStack);

			// Spill parameters to shadow space as needed. We want to do this early so that we can
			// start handling cleanup during exceptions early.
			spillParams(dest, &spillShadow);

			// Push all registers we need to preserve. We do it in reverse here to make the epilog
			// cheaper (only one prolog in functions, but many epilogs).
			const Nat maxToStore = 32; // Overkill for X64.
			Nat toStoreCount = 0;
			Reg toStore[maxToStore];
			for (RegSet::Iter i = toPreserve->begin(); i != toPreserve->end(); ++i) {
				assert(toStoreCount + 1 < maxToStore);
				toStore[toStoreCount++] = i.v();
			}

			for (Nat i = toStoreCount; i > 0; i--) {
				Reg r = asSize(toStore[i - 1], Size::sPtr);
				*dest << push(r);
				*dest << preserve(r);
			}

			// Allocate the stack frame. We use a second form of 'prolog' for that to emit the
			// proper metadata. We also mark this as the end of the prolog, and from here exceptions
			// will start being handled.
			Offset stackSize = layout->last() - Size::sPtr*toPreserve->count();
			*dest << prolog(engine())->alterSrc(ptrConst(stackSize));

			// Spill remaining parameters.
			spillParams(dest, &spillRemaining);
		}

		void WindowsLayout::emitEpilog(Listing *dest) {
			// Note: The instructions in the epilog are quite harshly specified in the ABI. As such,
			// we can not use the LEAVE instruction, or load rsp from rbp to avoid accidental
			// mis-alignment of the stack.

			// Restore stack pointer:
			Offset stackSize = layout->last() - Size::sPtr*toPreserve->count();
			*dest << add(ptrStack, ptrConst(stackSize));

			// Restore registers:
			for (RegSet::Iter i = toPreserve->begin(); i != toPreserve->end(); ++i)
				*dest << pop(asSize(i.v(), Size::sPtr));

			*dest << pop(ptrFrame);
		}

		static Nat useShadowSpace(Reg reg) {
			Reg shadowMap[] = { rcx, rdx, r8, r9 };
			for (Nat i = 0; i < ARRAY_COUNT(shadowMap); i++)
				if (same(reg, shadowMap[i]))
					return 2 + i;
			return 0;
		}

		static Size spillParams(Array<Offset> *out, Listing *l, Params *params, Offset varOffset) {
			// NOTE: We could avoid spilling primitives to memory, as we do not generally use those
			// registers from code generated for the generic platform. However, we would need to
			// save as soon as we perform a function call anyway, which is probably more expensive
			// than just spilling them unconditionally in the function prolog.

			Array<Var> *all = l->allParams();

			{
				// Start by computing the locations of parameters passed on the stack.
				Offset stackOffset = Offset::sPtr * (2 + 4); // rsp, rbp + 4 shadow space
				for (Nat i = 0; i < params->stackCount(); i++) {
					Nat paramId = params->stackParam(i).id(); // Should never be 'Param::returnId'
					Var var = all->at(paramId);

					out->at(var.key()) = stackOffset;
					stackOffset = (stackOffset + var.size().aligned()).alignAs(Size::sPtr);
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

				// If it is an integer register, use its shadow space.
				if (Nat shadowIndex = useShadowSpace(params->registerSrc(i))) {
					to = Offset::sPtr * shadowIndex;
					continue;
				}

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

		// Check so that enough shadow space is available to call destructors for all variables that
		// need destruction. If 'force' is true, then all variables are checked instead.
		static void ensureShadowSpace(Listing *l, Array<Offset> *offset, Array<Var> *vars, Int size, Bool force) {
			for (Nat i = 0; i < vars->count(); i++) {
				Var v = vars->at(i);

				if (force || (l->freeOpt(v) & freeOnBlockExit)) {
					Int off = offset->at(v.key()).v64();
					// Note: offset->last() is the total size of the stack.
					if (-off + size > offset->last().v64())
						offset->last() = Offset(-off + size);
				}
			}
		}

		// Check so that enough shadow space is available in the following situations:
		//
		// 1. When destroying a block, we must be able to call destructors for the
		//    object with the lowest address.
		// 2. When copying the result (fnRet or fnRetRef), we might need to call a
		//    copy-ctor. Furthermore, we might also need to temporarily store a word
		//    somewhere (also during 'epilog').
		static void ensureShadowSpace(Listing *l, Params *params, Array<Offset> *offset, Array<Var> *allVars) {
			const Int shadowSz = 0x20;

			Offset before = offset->last();

			// For #1 above, we can just iterate through all variables and make sure that the total
			// stack size is 0x20 larger than the object.
			ensureShadowSpace(l, offset, allVars, shadowSz, false);

			// For #2, we need to investigate the listing, to see which blocks are active when we
			// need additional stack space. If we would do it for all blocks, we would waste 32
			// bytes in all stack frames.
			// Note: We don't bother with sanity checks here. They are done when the transform
			// is executed later on anyway.

			// Extra space for storing the result through destructors:
			Int extraSpace = shadowSz + params->result().registerCount() * sizeof(void *);

			Block current;
			for (Nat i = 0; i < l->count(); i++) {
				Instr *instr = l->at(i);
				switch (instr->op()) {
				case op::prolog:
					current = l->root();
					break;
				case op::beginBlock:
					current = instr->src().block();
					if (instr->dest() == Operand()) {
						// No temporary register specified! Make sure we have space, and tell it to
						// spill to the stack.
						ensureShadowSpace(l, offset, l->allVars(current), 0x8, true);
						l->setInstr(i, instr->alterDest(ptrRel(ptrStack, Offset())));
					}
					break;
				case op::endBlock:
					// Note: destroys the block, but we care about that in case #1 above, so we
					// don't need to do anything more here. We only need to keep track of the
					// current block.
					current = l->parent(instr->src().block());
					break;
				case op::jmpBlock:
					// Note: Calls dtors. We take care of that in case #1 above, so we don't need to
					// do anything more here.
					break;
				case op::fnRet:
				case op::fnRetRef:
					// This is similar to the code for 'epilog'. The difference is that we consider
					// *all* variables here if we need to call a copy constructor.
					if (as<ComplexDesc>(l->result))
						ensureShadowSpace(l, offset, l->allVars(current), shadowSz, true);

					// Fall thru
				case op::epilog:
					// Ensure that all variables with dtors have shadow space, as well as one extra
					// word to store the result.
					for (Block b = current; b != Block(); b = l->parent(b)) {
						ensureShadowSpace(l, offset, l->allVars(b), extraSpace, false);
					}
					break;
				}
			}
		}

		Array<Offset> *WindowsLayout::computeLayout(Listing *l, Params *params, Nat spilled) {
			Array<Offset> *result = code::layout(l);

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

			ensureShadowSpace(l, params, result, all);

			if (result->last().v64() & 0xF) {
				// Need to be aligned to 64 bits. Otherwise, the SIMD operations used widely on
				// X86-64 will not work properly.
				result->last() = Offset(roundUp(result->last().v64(), 0x10));
			}

			// for (Nat i = 0; i < result->count(); i++)
			// 	PLN(i << L": " << result->at(i));

			return result;
		}

	}
}
