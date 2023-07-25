#include "stdafx.h"
#include "Arena.h"
#include "WindowsOutput.h"
#include "PosixOutput.h"
#include "Asm.h"
#include "AsmOut.h"
#include "RemoveInvalid.h"
#include "WindowsLayout.h"
#include "PosixLayout.h"
#include "Code/PosixEh/StackInfo.h"
#include "Code/WindowsEh/Seh.h"
#include "../Exception.h"

namespace code {
	namespace x64 {

		Arena::Arena() {
			dirtyRegs = new (this) RegSet();
		}

		Listing *Arena::transform(Listing *l) const {
#ifdef X64
#if defined(WINDOWS)
			code::eh::activateWindowsInfo(engine());
#elif defined(POSIX)
			code::eh::activatePosixInfo();
#endif
#endif

			// Remove unsupported OP-codes, replacing them with their equivalents.
			l = code::transform(l, this, new (this) RemoveInvalid(this));

			// Expand variables and function calls as well as function prolog and epilog.
			l = code::transform(l, this, layoutTfm());

			return l;
		}

		void Arena::output(Listing *src, Output *to) const {
			code::x64::output(src, to);
			to->finish();
		}

		LabelOutput *Arena::labelOutput() const {
			return new (this) LabelOutput(8);
		}

		void Arena::removeFnRegs(RegSet *from) const {
			for (RegSet::Iter i = dirtyRegs->begin(), end = dirtyRegs->end(); i != end; ++i)
				from->remove(*i);
		}

		Listing *Arena::redirect(Bool member, TypeDesc *result, Array<TypeDesc *> *params, Ref fn, Operand param) {
			Listing *l = new (this) Listing(this);

			// Generate a layout of all parameters so we can properly restore them later.
			Params *layout = layoutParams(result, params);

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

			// Call 'fn' to obtain the actual function to call.
			if (!param.empty())
				*l << fnParam(ptrDesc(engine()), param);
			*l << fnCall(fn, member, ptrDesc(engine()), ptrA);

			// Restore the registers.
			for (Nat i = 0; i < layout->registerCount(); i++) {
				Var v = vars->at(i);
				if (v != Var())
					*l << mov(asSize(layout->registerSrc(i), Size::sLong), v);
			}

			// Note: The epilog will preserve all registers in this case since there are no destructors to call!
			*l << epilog();
			*l << jmp(ptrA);

			return l;
		}

		Listing *Arena::engineRedirect(TypeDesc *result, Array<TypeDesc *> *params, Ref fn, Operand engine) {
			Listing *l = new (this) Listing(this);

			// Examine the parameters to see if we have room for an additional parameter without
			// spilling to the stack. We assume that no stack spilling is required since it vastly
			// simplifies the implementation of the redirect, and we expect it to be needed very
			// rarely. The functions that require an EnginePtr are generally small free-standing toS
			// functions that do not require many parameters. Functions that would require spilling
			// to the stack should probably be moved into some object anyway.
			Params *layout = layoutParams(result, params);

			// Shift all registers.
			Reg last = noReg;
			for (Nat i = layout->registerCount(); i > 0; i--) {
				Nat id = i - 1;

				Reg r = layout->registerSrc(id);
				// Interesting?
				if (fpRegister(r))
					continue;

				Param par = layout->registerParam(id);
				// Unused?
				if (par == Param()) {
					last = r;
					continue;
				}
				// Result parameter? Don't touch that!
				if (par.id() == Param::returnId()) {
					continue;
				}
				// All integer registers full?
				if (last == noReg)
					throw new (this) InvalidValue(S("Can not create an engine redirect for this function. ")
												S("It has too many (integer) parameters."));

				// Move the registers one step 'up'.
				*l << mov(last, r);
				last = r;
			}

			// Now, we can simply put the engine ptr inside the first register and jump on to the
			// function we actually wanted to call.
			*l << mov(last, engine);
			*l << jmp(fn);

			return l;
		}

		Reg Arena::functionDispatchReg() {
			return ptrA;
		}

		Params *Arena::layoutParams(TypeDesc *result, Array<TypeDesc *> *params) {
			Params *layout = createParams();
			layout->result(result);
			for (Nat i = 0; i < params->count(); i++)
				layout->add(i, params->at(i));
			return layout;
		}


		/**
		 * Windows version.
		 */

		WindowsArena::WindowsArena() {
			static const Reg dirty[] = {
				rax, rdx, rcx, r8, r9, r10, r11,
				xmm0, xmm1, xmm2, xmm3, xmm4, xmm5,
			};
			for (size_t i = 0; i < ARRAY_COUNT(dirty); i++)
				this->dirtyRegs->put(dirty[i]);
		}

		LabelOutput *WindowsArena::labelOutput() const {
			return new (this) WindowsLabelOut();
		}

		CodeOutput *WindowsArena::codeOutput(Binary *owner, LabelOutput *size) const {
			if (WindowsLabelOut *out = as<WindowsLabelOut>(size)) {
				return new (this) WindowsCodeOut(owner, out);
			} else {
				throw new (this) InternalError(S("A WindowsLabelOut instance is needed on 64-bit Windows."));
			}
		}

		Nat WindowsArena::firstParamId(MAYBE(TypeDesc *) desc) {
			if (!desc)
				return 2;

			Params *p = new (this) WindowsParams();
			p->result(desc);
			if (p->result().memoryRegister() == noReg)
				return 0;
			else
				return 1;
		}

		Operand WindowsArena::firstParamLoc(Nat id) {
			switch (id) {
			case 0:
				// In a register, first parameter.
				return ptrC;
			case 1:
				// In memory, second parameter.
				return ptrD;
			default:
				return Operand();
			}
		}

		code::Params *WindowsArena::createParams() const {
			return new (this) WindowsParams();
		}

		Layout *WindowsArena::layoutTfm() const {
			return new (this) WindowsLayout(this);
		}


		/**
		 * Posix version.
		 */

		PosixArena::PosixArena() {
			static const Reg dirty[] = {
				rax, rdi, rsi, rdx, rcx, r8, r9, r10, r11,
				xmm0, xmm1, xmm2, xmm3, xmm4, xmm5, xmm6, xmm7,
			};
			for (size_t i = 0; i < ARRAY_COUNT(dirty); i++)
				this->dirtyRegs->put(dirty[i]);
		}

		CodeOutput *PosixArena::codeOutput(Binary *owner, LabelOutput *size) const {
			return new (this) PosixCodeOut(owner, size->offsets, size->size, size->refs);
		}

		Nat PosixArena::firstParamId(MAYBE(TypeDesc *) desc) {
			if (!desc)
				return 2;

			Params *p = new (this) PosixParams();
			p->result(desc);
			if (p->result().memoryRegister() == noReg)
				return 0;
			else
				return 1;
		}

		Operand PosixArena::firstParamLoc(Nat id) {
			switch (id) {
			case 0:
				// In a register, first parameter.
				return ptrDi;
			case 1:
				// In memory, second parameter.
				return ptrSi;
			default:
				return Operand();
			}
		}

		code::Params *PosixArena::createParams() const {
			return new (this) PosixParams();
		}

		Layout *PosixArena::layoutTfm() const {
			return new (this) PosixLayout(this);
		}

	}
}
