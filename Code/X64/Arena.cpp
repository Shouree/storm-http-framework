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
			Params *layout = layoutParams(member, result, params);

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

		static Listing *engineRedirectSimple(Arena *arena, Params *layout, Ref fn, Operand engine) {
			// Simple case, where we have enough integer registers. We simply shift all registers
			// one step and we are done.
			Listing *l = new (arena) Listing(arena);

			Reg last = noReg;
			for (Nat i = layout->registerCount(); i > 0; i--) {
				Nat id = i - 1;

				Reg r = layout->registerSrc(id);
				// Don't bother with fp registers.
				if (fpRegister(r))
					continue;

				Param par = layout->registerParam(id);
				// Unused?
				if (par == Param()) {
					last = r;
					continue;
				}

				// Don't touch the result parameter.
				if (par.id() == Param::returnId())
					continue;

				// We should have an integer register to move into.
				assert(last != noReg, L"No available integer registers. Should have used complex redirect.");

				// Move it to the next register.
				*l << mov(last, r);
				last = r;
			}

			// Now, we can simply put the enginge ptr inside the first register and jump to the
			// function we actually wanted to call!
			*l << mov(last, engine);
			*l << jmp(fn);

			return l;
		}

		static Listing *engineRedirectComplex(Arena *arena,
											TypeDesc *result, Array<TypeDesc *> *params,
											Ref fn, Operand engine) {
			// Complex case. Use a "real" function since we need to allocate our own stack frame.
			Listing *l = new (arena) Listing(arena, false, result);
			TypeDesc *ptr = ptrDesc(arena->engine());

			*l << prolog();

			// Add engine parameter:
			*l << fnParam(ptr, engine);

			// Add parameters:
			for (Nat i = 0; i < params->count(); i++) {
				TypeDesc *paramDesc = params->at(i);
				if (as<ComplexDesc>(paramDesc)) {
					// Complex parameters are passed by pointer on these platforms. To avoid calling
					// copy ctors, we can thus pretend that the parameter is a pointer!

					// TODO: We can be even more eager by doing this to all parameters that have
					// their "pass in memory" flag set (on Windows, that is quite common).
					paramDesc = ptr;
				}
				Var param = l->createParam(paramDesc);
				*l << fnParam(paramDesc, param);
			}

			// Call the function:
			Var resultVar = l->createVar(l->root(), result);
			*l << fnCall(fn, false, result, resultVar);

			// Note: We could avoid a copy of the result when the result is passed as a pointer.
			*l << fnRet(resultVar);

			return l;
		}

		Listing *Arena::engineRedirect(TypeDesc *result, Array<TypeDesc *> *params, Ref fn, Operand engine) {
			// There are two variants of this function: One where all integer parameters are in
			// registers, and another where some of them need to go on the stack. The first version
			// generates simple machine code, while the second version requires a full wrapper
			// function.
			Params *layout = layoutParams(false, result, params);
			Bool useSimple = false;

			for (Nat i = 0; i < layout->registerCount(); i++) {
				// Don't bother with fp registers.
				if (fpRegister(layout->registerSrc(i)))
					continue;

				// If we find an empyt integer register, we can use the simple version.
				if (layout->registerParam(i) == Param()) {
					useSimple = true;
					break;
				}
			}

			// Now we know which one we have to use:
			if (useSimple) {
				return engineRedirectSimple(this, layout, fn, engine);
			} else {
				return engineRedirectComplex(this, result, params, fn, engine);
			}
		}

		Reg Arena::functionDispatchReg() {
			return ptrA;
		}

		Params *Arena::layoutParams(Bool member, TypeDesc *result, Array<TypeDesc *> *params) {
			Params *layout = createParams(member);
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
			// The this pointer is always in the first parameter register.
			if (!desc)
				return 1;

			return 0;
		}

		Operand WindowsArena::firstParamLoc(Nat id) {
			if (id != 0)
				return Operand();

			return ptrC;
		}

		code::Params *WindowsArena::createParams(Bool member) const {
			return new (this) WindowsParams(member);
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

		code::Params *PosixArena::createParams(Bool member) const {
			(void)member; // Not important on Posix.
			return new (this) PosixParams();
		}

		Layout *PosixArena::layoutTfm() const {
			return new (this) PosixLayout(this);
		}

	}
}
