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

		// Check so that enough shadow space is available in the following situations:
		//
		// 1. When destroying a block, we must be able to call destructors for the
		//    object with the lowest address.
		// 2. When copying the result (fnRet or fnRetRef), we might need to call a
		//    copy-ctor. Furthermore, we might also need to temporarily store a word
		//    somewhere (also during 'epilog').
		static void ensureShadowSpace(Listing *l, Array<Offset> *offset, Array<Var> *allVars) {
			const Int shadowSz = 0x20;

			// For #1 above, we can just iterate through all variables and make sure that the total
			// stack size is 0x20 larger than the object.
			for (Nat i = 0; i < allVars->count(); i++) {
				Var v = allVars->at(i);
				if (l->freeOpt(v) & freeOnBlockExit) {
					// TODO: This could be a function used in step #2 also.
					Int off = offset->at(v.key()).v64();
					if (-off + shadowSz > offset->last().v64())
						offset->last() = Offset(-off + shadowSz);
				}
			}

			// For #2, we need to investigate the listing...
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

			ensureShadowSpace(l, result, all);

			if (result->last().v64() & 0xF) {
				// Need to be aligned to 64 bits. Otherwise, the SIMD operations used widely on
				// X86-64 will not work properly.
				result->last() += Offset::sPtr;
			}

			return result;
		}

	}
}
