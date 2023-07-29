#include "stdafx.h"
#include "Seh.h"

#if defined(WINDOWS) && defined(X86)

#include "SafeSeh.h"
#include "Code/Binary.h"
#include "Code/FnState.h"
#include "Utils/Memory.h"

namespace code {
	namespace eh {

		/**
		 * Stack frame on x86:
		 */
		struct OnStack {
			// SEH chain.
			SEHFrame *prev;
			const void *sehHandler;

			// Pointer to the running code. This is so that we are able to extract the location of
			// the Binary object during unwinding.
			void *self;

			// The topmost active block and active variables.
			Nat activePartVars;

			// Current EBP points to this.
			void *lastEbp;

			// Get ebp.
			inline void *ebp() {
				return &lastEbp;
			}

			// Create from base pointer:
			static OnStack *fromEBP(void *ebp) {
				byte *p = (byte *)ebp;
				return (OnStack *)(p - OFFSET_OF(OnStack, lastEbp));
			}

			// Get SEH address.
			inline void *seh() {
				return &prev;
			}

			// Create from SEH pointer:
			static OnStack *fromSEH(void *seh) {
				byte *p = (byte *)seh;
				return (OnStack *)(p - OFFSET_OF(OnStack, prev));
			}
		};

		SehFrame extractFrame(_EXCEPTION_RECORD *er, void *frame, _CONTEXT *ctx, void *dispatch) {
			OnStack *onStack = OnStack::fromSEH(frame);

			SehFrame result;
			result.framePtr = onStack->ebp();
			result.binary = codeBinary(onStack->self);
			decodeFnState(onStack->activePartVars, result.part, result.activation);
			return result;
		}

		// Called when we catch an exception. Called from a shim in assembler located in SafeSeh.asm
		extern "C"
		void *x86SEHCleanup(OnStack *frame, storm::Nat cleanUntil, void *exception) {
			// Perform a partial cleanup of the frame:
			Binary *binary = codeBinary(frame->self);
			TODO(L"Partial cleanup needed!");
			return exception;
		}

		void resumeFrame(SehFrame &frame, Binary::Resume &resume, storm::RootObject *object,
						_CONTEXT *ctx, _EXCEPTION_RECORD *er) {
			OnStack *onStack = OnStack::fromEBP(frame->framePtr);

			// First, we need to unwind the stack:
			er->ExceptionFlags |= EXCEPTION_UNWINDING;
			x86Unwind(er, onStack->seh());
			er->ExceptionFlags &= EXCEPTION_UNWINDING;

			// Clear the noncontinuable flag. Otherwise, we can't continue from the exception.
			er->ExceptionFlags &= ~DWORD(EXCEPTION_NONCONTINUABLE);

			// Build a stack "frame" that executes 'x86EhEntry' and returns to the resume point with Eax
			// set as intended. This approach places all data in registers, so that data on the stack is
			// not clobbered if we need it. It is also nice, as we don't have to think too carefully about
			// calling conventions and stack manipulations.
			ctx->Ebp = (UINT_PTR)frame->framePtr;
			ctx->Esp = ctx->Ebp + resume.stackOffset;

			// Run.
			ctx->Eip = (UINT_PTR)&x86HandleException;
			// Return to.
			ctx->Edx = (UINT_PTR)resume.ip;
			// Exception.
			ctx->Eax = (UINT_PTR)object;
			// Current part.
			ctx->Ebx = (UINT_PTR)resume.cleanUntil;
			// Current frame.
			ctx->Ecx = (UINT_PTR)onStack;

			// Note: We also need to restore the EH chain (fs:[0]). The ASM shim does this for us.
		}


		// The SEH frame on the stack.
		struct SEHFrame {
			// SEH chain.
			SEHFrame *prev;
			const void *sehHandler;

			// Pointer to the running code. This is so that we are able to extract the location of
			// the Binary object during unwinding.
			void *self;

			// The topmost active block and active variables.
			Nat activePartVars;

			// Current EBP points to this.
			void *lastEbp;

			// Get EBP.
			inline void *ebp() {
				return &lastEbp;
			}

			// Create from EBP.
			static SEHFrame *fromEbp(void *ebp) {
				byte *p = (byte *)ebp;
				return (SEHFrame *)(p - OFFSET_OF(SEHFrame, lastEbp));
			}

			// Create from SEH frame.
			static SEHFrame *fromSEH(void *seh) {
				byte *p = (byte *)seh;
				return (SEHFrame *)(p - OFFSET_OF(SEHFrame, prev));
			}

			// Cleanup this frame.
			void cleanup() {
				Binary *owner = codeBinary(self);
				if (owner) {
					Frame f(this);
					owner->cleanup(f);
				} else {
					WARNING(L"Using SEH, but no link to the metadata provided!");
				}
			}

			// Partial cleanup of this frame. Will set 'activePart' accordingly.
			void cleanup(Nat until) {
				Binary *owner = codeBinary(self);
				if (owner) {
					Frame f(this);
					Nat part = owner->cleanup(f, until);
					activePartVars = encodeFnState(part, f.activation);
				} else {
					WARNING(L"Using SEH, but no link to the metadata provided!");
				}
			}

			// Check if we want to catch any object at all in this frame.
			bool hasCatch() {
				Binary *owner = codeBinary(self);
				if (!owner)
					return false;
				return owner->hasCatch();
			}

			// Check if we want to catch this exception.
			bool hasCatch(storm::RootObject *exception, Binary::Resume &resume) {
				Binary *owner = codeBinary(self);
				if (!owner)
					return false;
				Nat part, activation;
				decodeFnState(activePartVars, part, activation);
				return owner->hasCatch(part, exception, resume);
			}

		};


	}
}

#endif
