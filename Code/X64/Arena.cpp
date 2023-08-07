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
#include "Code/Exception.h"
#include "Core/GcBitset.h"

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

		static Listing *engineRedirectSimple(Arena *arena, Params *before, Params *after, Ref fn, Operand engine) {
			// Simple case, where we don't have to modify the stack contents.
			// We simply shuffle the registers around to match the new layout.
			Listing *l = new (arena) Listing(arena);

			// Note: Iterate backwards so that we can move registers in a chain without worrying
			// about overwriting them.
			for (Nat i = after->registerCount(); i > 0; i--) {
				Nat id = i - 1;
				Param par = after->registerParam(id);
				Reg to = after->registerSrc(id);

				if (par.empty())
					continue;

				// Is it the parameter where we shall put the engine?
				if (par.id() == 0) {
					*l << mov(to, engine);
					continue;
				}

				// Find the register in 'before' to figure out its source. A simple linear search is
				// fine here, since we have well below 10 registers to look through.
				Nat srcId = before->registerCount();
				Nat lookFor = par.id();
				if (lookFor != Param::returnId())
					lookFor--;

				for (Nat j = 0; j < before->registerCount(); j++) {
					Param p = before->registerParam(j);
					if (p.id() == lookFor && p.offset() == par.offset()) {
						srcId = j;
						break;
					}
				}

				assert(srcId < before->registerCount(), L"Should have used the complex redirect!");

				// Move the register!
				Reg from = before->registerSrc(srcId);
				if (!same(to, from))
					*l << mov(to, from);
			}

			*l << jmp(fn);
			return l;
		}

		static Listing *engineRedirectComplex(Arena *arena, Params *layout,
											TypeDesc *result, Array<TypeDesc *> *params,
											Ref fn, Operand engine) {
			// Complex case. Use a "real" function since we need to allocate our own stack frame.
			Listing *l = new (arena) Listing(arena, false, result);
			TypeDesc *ptr = ptrDesc(arena->engine());

			// Store which parameters are passed by pointer.
			GcBitset *byPointer = allocBitset(arena->engine(), params->count());
			for (Nat i = 0; i < layout->totalCount(); i++)
				byPointer->set(i, layout->totalParam(i).inMemory());

			*l << prolog();

			// Add engine parameter:
			*l << fnParam(ptr, engine);

			// Add parameters:
			for (Nat i = 0; i < params->count(); i++) {
				TypeDesc *paramDesc = params->at(i);
				if (byPointer->has(i)) {
					// This parameter is passed by a pointer in memory. This means that we can treat
					// it as if it were a pointer to avoid copying it once more!
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
			// There are two variants of this function: One where the stack is identical before and
			// after adding the Engine parameter, and one where it is different. The first version
			// generates simple machine code (it only needs to re-organize registers), while the
			// second version requires a proper wrapper function.
			Params *before = layoutParams(false, result, params);

			Params *after = createParams(false);
			after->result(result);
			after->add(0, ptrDesc(this->engine()));
			for (Nat i = 0; i < params->count(); i++)
				after->add(i + 1, params->at(i));

			// Check if we can use the simple one (if the stack layout is the same), or if we need
			// the complex one.
			Bool useSimple = before->stackCount() == after->stackCount();
			if (useSimple) {
				for (Nat i = 0; i < before->stackCount(); i++) {
					Param beforeParam = before->stackParam(i);
					Param afterParam = after->stackParam(i).withId(beforeParam.id());

					if (beforeParam != afterParam) {
						useSimple = false;
						break;
					}
				}
			}

			if (useSimple) {
				return engineRedirectSimple(this, before, after, fn, engine);
			} else {
				return engineRedirectComplex(this, before, result, params, fn, engine);
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
