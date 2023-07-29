#include "stdafx.h"
#include "Seh64.h"
#include "Seh.h"

#if defined(WINDOWS) && defined(X64)

#include "Code/X64/Asm.h"
#include "Code/FnState.h"
#include "Gc/Gc.h"
#include "Gc/CodeTable.h"

namespace code {
	namespace eh {

		RUNTIME_FUNCTION *exceptionCallback(void *pc, void *base) {
			storm::CodeTable &table = storm::codeTable();

			void *found = table.find(pc);
			if (!found)
				return null;

			byte *code = (byte *)found;
			size_t size = storm::Gc::codeSize(code);

			Nat startOffset = Nat(size_t(code) - size_t(base));

			// Note: The format that is manipulated here matches what WindowsOutput creates for us
			// in its constructor. What is done here probably makes more sense in light of the code
			// there.

			// EH offset is stored at the end of the allocation:
			Nat ehOffset = *(Nat *)(code + size - sizeof(Nat));
			Nat dataStart = ehOffset + Nat(sizeof(RUNTIME_FUNCTION));

			// If pc is after where we will place the "EndAddress", then we simply return null. This
			// is to not accidentally confuse the unwinding logic if we get an exception during the
			// small shim in the end of the allocation.
			if (size_t(pc) > size_t(code) + ehOffset)
				return null;

			// Find and update the RUNTIME_FUNCTION in the function. Note: we need to update the
			// offsets inside it, since it might have moved since we used it last.
			RUNTIME_FUNCTION *fn = (RUNTIME_FUNCTION *)(code + ehOffset);
			fn->BeginAddress = startOffset;
			// Note: a bit too late, but it is cumbersome to figure out exactly where.
			fn->EndAddress = startOffset + ehOffset;
			fn->UnwindData = startOffset + dataStart;

			// Find and update the address of the handler function as well:
			UnwindInfo *uwInfo = (UnwindInfo *)&code[dataStart];
			Nat exCodeCount = roundUp(uwInfo->unwindCount, byte(2));
			*(DWORD *)(code + dataStart + sizeof(UnwindInfo) + exCodeCount*2)
				= startOffset + Nat(size) - 10;

			// For debugging:
			// PVAR(base);
			PVAR(found);
			PVAR((void *)(size_t(pc) - size_t(found)));
			// PVAR(fn);
			for (Nat i = 0; i < size - ehOffset; i++) {
				if (i % 8 == 0)
					PNN(L"\n" << i << L":");
				PNN(L" " << toHex(((byte *)fn)[i]));
			}
			PLN(L"");
			PLN(L"");

			return fn;
		}

		// From the Windows documentation:
		// https://learn.microsoft.com/en-us/cpp/build/exception-handling-x64?view=msvc-170
		struct DispatchContext {
			size_t pc;
			size_t base;
			RUNTIME_FUNCTION *fnEntry;
			size_t establisherFrame;
			size_t targetIp;
			CONTEXT *contextRecord;
			void *languageHandler;
			void *handlerData;
		};

		/**
		 * Description of a single entry in the block table.
		 */
		struct FnBlock {
			// At what offset do we start?
			Nat offset;

			// What block?
			Nat block;
		};

		// Get the block data from a pointer just before the exception metadata at the end:
		static const FnBlock *getBlocks(const size_t *count) {
			const FnBlock *end = (const FnBlock *)count;
			return end - *count;
		}

		// Compare object.
		struct BlockCompare {
			bool operator()(const FnBlock &a, Nat offset) const {
				return a.offset < offset;
			}
		};

		SehFrame extractFrame(_EXCEPTION_RECORD *er, void *frame, _CONTEXT *ctx, void *dispatch) {
			DispatchContext *dispatchContext = (DispatchContext *)dispatch;
			size_t endOfContext = (size_t)dispatchContext->handlerData;

			// Now, endOfContext is a pointer to the byte after we stored the pointer to this
			// function. Based on that, we can find the start of the GcCode since we know how much
			// is stored after that point in the code. See Code/X64/WindowsOutput.cpp for details.
			// Note: 6 is the size of the jump operation we use as a shim to call the EH function.
			size_t endOfCode = endOfContext + 6 + sizeof(Nat);
			endOfCode = roundUp(endOfCode, sizeof(void *));

			// Now we can retrieve the GcCode! Due to the object layout, it is right after the code
			// portion (when properly aligned):
			GcCode *refs = (GcCode *)endOfCode;

			SehFrame result;
			result.framePtr = frame;
			result.binary = code::codeBinaryImpl(refs);

			// We can also find the metadata table at the end of the binary:
			// The last Nat is the size of the EH data:
			Nat *ehOffset = ((Nat *)endOfCode) - 1;

			// The binary also contains the start of the code:
			size_t codeStart = size_t(result.binary->address());
			PVAR((void *)codeStart);

			// Now, we can compute the start of the EH data and extract the block table:
			size_t startOfEhData = codeStart + *ehOffset;

			// Block count is just before there:
			size_t *blockCount = (size_t *)(startOfEhData - sizeof(size_t));
			const FnBlock *blocks = getBlocks(blockCount);

			// Finally, we can look up the proper entry in the table:
			Nat offset = Nat(dispatchContext->pc - codeStart);
			Nat active = Block().key();

			// Note: In case we have multiple equal elements, we pick the last one. Hence upper bound.
			// Note: if there is an entry where 'offset == blocks[i].offset', we shall not select
			// that one since 'pc' points to the first instruction that was not executed.
			// Note: if there are multiple elements that are equal, we need to pick the last one.
			TODO(L"Fix last note above");
			const FnBlock *found = std::upper_bound(blocks, blocks + *blockCount, offset, BlockCompare());
			if (found != blocks) {
				found--;
				active = found->block;
			}

			code::decodeFnState(active, result.part, result.activation);

			return result;
		}

		// Note: This is not always present in the Windows headers.
		extern "C"
		NTSYSAPI VOID RtlUnwindEx(PVOID targetFrame, PVOID targetIp, PEXCEPTION_RECORD er,
								PVOID returnValue, PCONTEXT context, void *history);


		void resumeFrame(SehFrame &frame, Binary::Resume &resume, storm::RootObject *object,
						_CONTEXT *ctx, _EXCEPTION_RECORD *er) {

			PLN(L"About to unwind!");

			TODO(L"Handle partial destruction of current frame!");

			er->ExceptionFlags |= EXCEPTION_UNWINDING;
			er->ExceptionFlags &= ~DWORD(EXCEPTION_NONCONTINUABLE);
			// TODO: It would be nice to find a history table for RtlUnwind, so that it can avoid to
			// find the handlers again.
			RtlUnwindEx(frame.framePtr, resume.ip, er, object, ctx, NULL);

			// Expected to not return...
			dbg_assert(false, L"Failed to unwind the stack!");
		}

	}
}

#endif

namespace code {
	namespace eh {

		Nat win64Register(Reg reg) {
			using namespace code::x64;
			static const Reg order[] = {
				rax, rcx, rdx, rbx, ptrStack, ptrFrame, rsi, rdi,
				r8, r9, r10, r11, r12, r13, r14, r15
			};
			for (Nat i = 0; i < ARRAY_COUNT(order); i++) {
				if (same(reg, order[i]))
					return i;
			}

			assert(false, L"Register not supported!");
		}

	}
}
