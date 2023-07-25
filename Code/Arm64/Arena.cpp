#include "stdafx.h"
#include "Arena.h"
#include "Asm.h"
#include "AsmOut.h"
#include "Output.h"
#include "Code/Listing.h"
#include "Code/Output.h"
#include "RemoveInvalid.h"
#include "Layout.h"
#include "Params.h"
#include "../Exception.h"
#include "Code/PosixEh/StackInfo.h"

namespace code {
	namespace arm64 {

		Arena::Arena() {}

		Listing *Arena::transform(Listing *l) const {
#if defined(POSIX) && defined(ARM64)
			code::eh::activatePosixInfo();
#endif

			// Remove unsupported OP-codes, replacing them with their equivalents.
			l = code::transform(l, this, new (this) RemoveInvalid());

			// Expand variables and function calls as well as function prolog and epilog.
			l = code::transform(l, this, new (this) Layout());

			return l;
		}

		void Arena::output(Listing *src, Output *to) const {
			code::arm64::output(src, to);
			to->finish();
		}

		LabelOutput *Arena::labelOutput() const {
			return new (this) LabelOutput(8);
		}

		CodeOutput *Arena::codeOutput(Binary *owner, Array<Nat> *offsets, Nat size, Nat refs) const {
			return new (this) CodeOut(owner, offsets, size, refs);
		}

		void Arena::removeFnRegs(RegSet *from) const {
			for (size_t i = 0; i < fnDirtyCount; i++)
				from->remove(fnDirtyRegs[i]);
		}

		Listing *Arena::redirect(Bool member, TypeDesc *result, Array<TypeDesc *> *params, Ref fn, Operand param) {
			Listing *l = new (this) Listing(this);

			// Generate a layout of all parameters so we can properly restore them later.
			Params *layout = layoutParams(result, params);
			Result res = layout->result();

			// Note: We want to use the 'prolog' and 'epilog' functionality so that exceptions from
			// 'fn' are able to propagate through this stub properly.
			*l << prolog();

			// Store the registers used for parameters inside variables on the stack.
			Array<Var> *vars = new (this) Array<Var>(layout->registerCount(), Var());
			for (Nat i = 0; i < layout->registerCount(); i++) {
				if (layout->registerParam(i) != Param()) {
					Var &v = vars->at(i);
					v = l->createVar(l->root(), Size::sLong);
					*l << mov(v, asSize(layout->registerSrc(i), Size::sLong));
				}
			}

			// If result is in memory, we need to save/restore x8 as well!
			Var resVar;
			if (res.memoryRegister() != noReg) {
				resVar = l->createVar(l->root(), Size::sPtr);
				*l << mov(resVar, ptrr(8));
			}

			// Call 'fn' to obtain the actual function to call.
			if (!param.empty())
				*l << fnParam(ptrDesc(engine()), param);
			*l << fnCall(fn, member, ptrDesc(engine()), ptrA);

			// Save the output from x0 to another register, otherwise parameters will overwrite it. x17 is good.
			*l << mov(ptrr(17), ptrA);

			// Restore the registers.
			for (Nat i = 0; i < layout->registerCount(); i++) {
				Var v = vars->at(i);
				if (v != Var())
					*l << mov(asSize(layout->registerSrc(i), Size::sLong), v);
			}

			if (res.memoryRegister() != noReg) {
				*l << mov(ptrr(8), resVar);
			}

			// Note: The epilog will preserve all registers in this case since there are no destructors to call!
			*l << epilog();
			*l << jmp(ptrr(17));

			return l;
		}

		static Reg nextIntReg(Params *params, Nat &id) {
			while (id > 0) {
				Reg r = params->registerSrc(--id);
				if (r == noReg || isVectorReg(r))
					continue;

				if (params->registerParam(id) == Param())
					continue;

				return r;
			}

			return noReg;
		}

		Listing *Arena::engineRedirect(TypeDesc *result, Array<TypeDesc *> *params, Ref fn, Operand engine) {
			Listing *l = new (this) Listing(this);

			// Examine parameters to see what we need to do. Aarch64 is a bit tricky since some
			// register usage is "aligned" to even numbers. For this reason, we produce two layouts
			// and "diff" them.
			Params *called = new (this) Params();
			Params *toCall = new (this) Params();
			toCall->add(0, Primitive(primitive::pointer, Size::sPtr, Offset()));
			for (Nat i = 0; i < params->count(); i++) {
				called->add(i + 1, params->at(i));
				toCall->add(i + 1, params->at(i));
			}

			if (toCall->stackCount() > 0 || called->stackCount() > 0)
				throw new (this) InvalidValue(S("Can not create an engine redirect for this function. ")
											S("It has too many (integer) parameters."));


			// Traverse backwards to ensure we don't overwrite anything.
			Nat calledId = called->registerCount();
			Nat toCallId = toCall->registerCount();
			while (true) {
				// Find the next source register:
				Reg srcReg = nextIntReg(called, calledId);
				Reg destReg = nextIntReg(toCall, toCallId);

				if (srcReg == noReg)
					break;
				assert(destReg, L"Internal inconsistency when creating a redirect stub!");
				*l << mov(destReg, srcReg);
			}

			// Now, we can simply put the engine ptr in x0 and jump to the function we need to call.
			*l << mov(ptrr(0), engine);
			*l << jmp(fn);

			return l;
		}

		Nat Arena::firstParamId(MAYBE(TypeDesc *) desc) {
			if (!desc)
				return 1;
			return 0;
		}

		Operand Arena::firstParamLoc(Nat id) {
			return ptrr(0);
		}

		Reg Arena::functionDispatchReg() {
			return ptrr(17); // We can also use x16. x17 is nice as we use that elsewhere.
		}

	}
}
