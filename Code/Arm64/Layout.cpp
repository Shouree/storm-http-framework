#include "stdafx.h"
#include "Layout.h"
#include "../Listing.h"
#include "../Binary.h"
#include "../Layout.h"

namespace code {
	namespace arm64 {

		Layout::Layout(Binary *owner) : owner(owner) {}

		void Layout::before(Listing *dest, Listing *src) {
			// Find parameters.
			Array<Var> *p = src->allParams();
			params = new (this) Params();
			for (Nat i = 0; i < p->count(); i++) {
				params->add(i, src->paramDesc(p->at(i)));
			}

			PVAR(src);
			layout = code::arm64::layout(src, params, 0);

			PLN(L"Layout:");
			for (Nat i = 0; i < layout->count(); i++)
				PLN(i << L": " << layout->at(i));
		}

		void Layout::during(Listing *dest, Listing *src, Nat id) {
			*dest << src->at(id);
		}

		void Layout::after(Listing *dest, Listing *src) {
			*dest << alignAs(Size::sPtr);
			*dest << dest->meta();

			TODO(L"Add metadata!");

			// Owner.
			*dest << dat(objPtr(owner));
		}

		Array<Offset> *layout(Listing *src, Params *params, Nat spilled) {
			Array<Offset> *result = code::layout(src);
			Array<Var> *paramVars = src->allParams();

			// A stack frame on Arm64 is as follows. Offsets are relative sp and x29 (frame
			// ptr). The prolog will make sure that we can use x29 as the frame pointer in the
			// generated code. This is important as the function-call code adjusts sp to make room
			// for parameters, and we still need the ability to access local variables at that
			// point.
			//
			// ...
			// param1
			// param0    <- old sp
			// localN
			// ...
			// local0
			// spilled-paramN
			// ...
			// spilled-param0
			// spilledN
			// ...
			// spilled0
			// old-x30
			// old-x29   <- sp, x29

			// Account for parameters that are in registers and need to be spilled to the stack.
			// TODO: It would be nice to avoid this, but we need more advanced register allocation
			// for that to work properly.
			Nat spilledParams = 0;
			for (Nat i = 0; i < params->registerCount(); i++)
				if (params->registerAt(i) != Param())
					spilledParams++;

			// TODO: We might need to align spilled registers to a 16-byte boundary to make stp and
			// ldp work properly.

			// Account for the "stack frame" (x30 and x29):
			spilled += 2;

			// Adjust "result" to account for spilled registers.
			Nat adjust = (spilled + spilledParams) * 8;
			for (Nat i = 0; i < result->count(); i++) {
				result->at(i) += Offset(adjust);
			}

			// Fill in the parameters that need to be spilled to the stack.
			Nat spillCount = spilled;
			for (Nat i = 0; i < params->registerCount(); i++) {
				Param p = params->registerAt(i);
				// Some params are "padding".
				if (p == Param())
					continue;

				result->at(paramVars->at(p.id()).key()) = Offset(spillCount * 8);
				spillCount += 1;
			}

			// Fill in the offset of parameters passed on the stack.
			for (Nat i = 0; i < params->stackCount(); i++) {
				Offset off(params->stackOffset(i));
				off += result->last(); // Increase w. size of entire stack frame.
				result->at(paramVars->at(params->stackParam(i)).key()) = off;
			}

			// Finally: ensure that the size of the stack is rounded up to a multiple of 16 bytes.
			if (result->last().v64() & 0xF)
				result->last() += Offset::sPtr;

			return result;
		}
	}
}
