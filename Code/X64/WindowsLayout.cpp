#include "stdafx.h"
#include "WindowsLayout.h"
#include "Code/Layout.h"

namespace code {
	namespace x64 {

		WindowsLayout::WindowsLayout(const Arena *arena) : Layout(arena) {}

		Offset WindowsLayout::resultParam() {
			Result r = params->result();
			if (same(r.memoryRegister(), ptrC)) {
				// Shadow space for rcx.
				return Offset::sPtr * 2;
			} else if (same(r.memoryRegister(), ptrD)) {
				// Shadow space for rcx.
				return Offset::sPtr * 3;
			} else {
				dbg_assert(false, L"Should not need resultparam now.");
				return Offset();
			}
		}

		void WindowsLayout::saveResult(Listing *dest) {
			const Result &result = params->result();

			// We have reserved space to store these right above the shadow space!
			if (result.memoryRegister() != noReg) {
				*dest << mov(ptrRel(ptrStack, Offset::sPtr * 4), ptrA);
			} else {
				for (Nat i = 0; i < result.registerCount(); i++) {
					*dest << mov(ptrRel(ptrStack, Offset::sPtr * Int(i + 4)),
								asSize(result.registerAt(i), Size::sPtr));
				}
			}
		}

		void WindowsLayout::restoreResult(Listing *dest) {
			const Result &result = params->result();

			// We have reserved space to store these right above the shadow space!
			if (result.memoryRegister() != noReg) {
				*dest << mov(ptrA, ptrRel(ptrStack, Offset::sPtr * 4));
			} else {
				for (Nat i = 0; i < result.registerCount(); i++) {
					*dest << mov(asSize(result.registerAt(i), Size::sPtr),
								ptrRel(ptrStack, Offset::sPtr * Int(i + 4)));
				}
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
			// If the last instruction was a call instruction, we need to insert a nop here.
			// Otherwise, the exception handling code thinks that we would return to the function
			// epilog and will not call our unwind handler. This is fairly rare, since in most cases
			// we would have emitted code to destroy variables right before here, but if we have
			// variables that are only destroyed when exceptions are thrown, we would miss them if
			// we don't have a nop here.
			padCallWithNop(dest);

			// Note: The instructions in the epilog are quite harshly specified in the ABI. As such,
			// we can not use the LEAVE instruction, or load rsp from rbp to avoid accidental
			// mis-alignment of the stack.

			// Restore stack pointer:
			Offset stackSize = layout->last() - Size::sPtr*toPreserve->count();
			if (stackSize.v64() != 0)
				*dest << add(ptrStack, ptrConst(stackSize));

			// Restore registers:
			for (RegSet::Iter i = toPreserve->begin(); i != toPreserve->end(); ++i)
				*dest << pop(asSize(i.v(), Size::sPtr));

			*dest << pop(ptrFrame);
		}

		static Nat useShadowSpace(Reg reg) {
			// Note: Both rcx and xmm0 can not be used simultaneously on Windows, so we can always
			// spill like this.
			Reg shadowMap1[] = { rcx, rdx, r8, r9 };
			Reg shadowMap2[] = { xmm0, xmm1, xmm2, xmm3 };
			for (Nat i = 0; i < ARRAY_COUNT(shadowMap1); i++) {
				if (same(reg, shadowMap1[i]))
					return 2 + i;
				if (same(reg, shadowMap2[i]))
					return 2 + i;
			}
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

		enum EnsureFlags {
			// Assume we are only calling destructors. I.e., if no destructors need to be executed,
			// then we won't modify the shadow space at all.
			ensureOnlyDtors = 0x0,

			// Assume that we will need the extra space always, but it is fine to clobber any
			// variables that don't need their destructors to be called.
			ensureAllowClobber = 0x1,

			// Assume that we will need the extra space, but disallow clobbering any active
			// variables.
			ensureNoClobber = 0x2,
		};

		bool shouldPrint = false;

		// Check so that enough shadow space is available to call destructors for all variables that
		// need destruction. If 'force' is true, then all variables are checked instead.
		static void ensureShadowSpace(Listing *l, Array<Offset> *offset, Array<Var> *vars, Int size, EnsureFlags flags) {
			for (Nat i = 0; i < vars->count(); i++) {
				Var v = vars->at(i);

				if ((flags >= ensureNoClobber) || (l->freeOpt(v) & freeOnBlockExit)) {
					Int off = offset->at(v.key()).v64();
					// Clamp to zero: we need to call something. Even if the variable is located
					// above return addr, make sure to not clobber return addr and stored ebp!
					if (off > 0)
						off = 0;
					// Note: offset->last() is the total size of the stack.
					if (-off + size > offset->last().v64())
						offset->last() = Offset(-off + size);
				}
			}

			// If we always execute something, make sure we don't accidentally clobber rbp and
			// return address.
			if (flags >= ensureAllowClobber)
				if (size > offset->last().v64())
					offset->last() = Offset(size);
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

			// Offset before = offset->last();

			// For #1 above, we can just iterate through all variables and make sure that the total
			// stack size is 0x20 larger than the object, for all objects that have destructors.
			ensureShadowSpace(l, offset, allVars, shadowSz, ensureOnlyDtors);

			// For #2, we need to investigate the listing, to see which blocks are active when we
			// need additional stack space. If we would do it for all blocks, we would waste 32
			// bytes in all stack frames.
			// Note: We don't bother with sanity checks here. They are done when the transform
			// is executed later on anyway.

			// Extra space for storing the result through destructors:
			Int extraSpace = shadowSz;
			if (params->result().memoryRegister())
				extraSpace += sizeof(void *); // We need to store rax during destructors
			else
				extraSpace += params->result().registerCount() * sizeof(void *);

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
						ensureShadowSpace(l, offset, l->allVars(current), 0x8, ensureNoClobber);
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
					// Note: This is only necessary when we call a copy ctor, we are fine
					// in cases where we return in memory but can use memcpy. Thus, the following
					// is too conservative:
					// if (params->result().memoryRegister() != noReg)
					if (as<ComplexDesc>(l->result)) {
						// TODO: Most likely, we can give this function 'ensureAllowClobber'
						// instead. It is generally fine if we clobber variables without destructors
						// (the complex type we have here most likely has a destructor).
						ensureShadowSpace(l, offset, l->allVars(current), shadowSz, ensureNoClobber);
					}

					// Fall through
				case op::epilog:
					// Ensure that all variables with dtors have shadow space, as well as one extra
					// word to store the result.
					for (Block b = current; b != Block(); b = l->parent(b)) {
						ensureShadowSpace(l, offset, l->allVars(b), extraSpace, ensureOnlyDtors);
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
