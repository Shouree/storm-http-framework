#include "stdafx.h"
#include "Arena.h"

namespace code {
	namespace arm64 {

		Arena::Arena() {}

		Listing *Arena::transform(Listing *l, Binary *owner) const {
// #if defined(POSIX) && defined(X64)
// 			activateInfo();
// #endif

// 			// Remove unsupported OP-codes, replacing them with their equivalents.
// 			l = code::transform(l, this, new (this) RemoveInvalid());

// 			// Expand variables and function calls as well as function prolog and epilog.
// 			l = code::transform(l, this, new (this) Layout(owner));

// 			return l;
			return null;
		}

		void Arena::output(Listing *src, Output *to) const {
			// code::x64::output(src, to);
		}

		LabelOutput *Arena::labelOutput() const {
			// return new (this) LabelOutput(8);
		}

		CodeOutput *Arena::codeOutput(Binary *owner, Array<Nat> *offsets, Nat size, Nat refs) const {
			// return new (this) CodeOut(owner, offsets, size, refs);
		}

		void Arena::removeFnRegs(RegSet *from) const {
			// RegSet *r = fnDirtyRegs(engine());
			// for (RegSet::Iter i = r->begin(); i != r->end(); ++i)
			// 	from->remove(i.v());
		}

		Listing *Arena::redirect(Bool member, TypeDesc *result, Array<TypeDesc *> *params, Ref fn, Operand param) {
			// Listing *l = new (this) Listing(this);

			// // Generate a layout of all parameters so we can properly restore them later.
			// Params *layout = layoutParams(result, params);

			// // Note: We want to use the 'prolog' and 'epilog' functionality so that exceptions from
			// // 'fn' are able to propagate through this stub properly.
			// *l << prolog();

			// // Store the registers used for parameters inside variables on the stack.
			// Array<Var> *vars = new (this) Array<Var>(layout->registerCount(), Var());
			// for (Nat i = 0; i < layout->registerCount(); i++) {
			// 	if (layout->registerAt(i) != Param()) {
			// 		Var &v = vars->at(i);
			// 		v = l->createVar(l->root(), Size::sLong);
			// 		*l << mov(v, asSize(layout->registerSrc(i), Size::sLong));
			// 	}
			// }

			// // Call 'fn' to obtain the actual function to call.
			// if (!param.empty())
			// 	*l << fnParam(ptrDesc(engine()), param);
			// *l << fnCall(fn, member, ptrDesc(engine()), ptrA);

			// // Restore the registers.
			// for (Nat i = 0; i < layout->registerCount(); i++) {
			// 	Var v = vars->at(i);
			// 	if (v != Var())
			// 		*l << mov(asSize(layout->registerSrc(i), Size::sLong), v);
			// }

			// // Note: The epilog will preserve all registers in this case since there are no destructors to call!
			// *l << epilog();
			// *l << jmp(ptrA);

			// return l;
		}

		Listing *Arena::engineRedirect(TypeDesc *result, Array<TypeDesc *> *params, Ref fn, Operand engine) {
			// Listing *l = new (this) Listing(this);

			// // Examine the parameters to see if we have room for an additional parameter without
			// // spilling to the stack. We assume that no stack spilling is required since it vastly
			// // simplifies the implementation of the redirect, and we expect it to be needed very
			// // rarely. The functions that require an EnginePtr are generally small free-standing toS
			// // functions that do not require many parameters. Functions that would require spilling
			// // to the stack should probably be moved into some object anyway.
			// Params *layout = layoutParams(result, params);

			// // Shift all registers.
			// Reg last = noReg;
			// for (Nat i = layout->registerCount(); i > 0; i--) {
			// 	Nat id = i - 1;

			// 	Reg r = layout->registerSrc(id);
			// 	// Interesting?
			// 	if (fpRegister(r))
			// 		continue;

			// 	Param par = layout->registerAt(id);
			// 	// Unused?
			// 	if (par == Param()) {
			// 		last = r;
			// 		continue;
			// 	}
			// 	// Result parameter? Don't touch that!
			// 	if (par.id() == Param::returnId) {
			// 		continue;
			// 	}
			// 	// All integer registers full?
			// 	if (last == noReg)
			// 		throw new (this) InvalidValue(S("Can not create an engine redirect for this function. ")
			// 									S("It has too many (integer) parameters."));

			// 	// Move the registers one step 'up'.
			// 	*l << mov(last, r);
			// 	last = r;
			// }

			// // Now, we can simply put the engine ptr inside the first register and jump on to the
			// // function we actually wanted to call.
			// *l << mov(last, engine);
			// *l << jmp(fn);

			// return l;
		}

		Nat Arena::firstParamId(MAYBE(TypeDesc *) desc) {
			// if (!desc)
			// 	return 2;

			// return result(desc)->memory ? 1 : 0;
		}

		Operand Arena::firstParamLoc(Nat id) {
			// switch (id) {
			// case 0:
			// 	// In a register, first parameter.
			// 	return ptrDi;
			// case 1:
			// 	// In memory, second parameter.
			// 	return ptrSi;
			// default:
			// 	return Operand();
			// }
		}

	}
}
