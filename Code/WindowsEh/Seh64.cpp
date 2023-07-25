#include "stdafx.h"
#include "Seh64.h"

#if defined(WINDOWS) && defined(X64)

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

			// EH offset is stored at the end of the allocation:
			Nat ehOffset = *(Nat *)(code + size - sizeof(Nat));
			Nat dataStart = ehOffset + Nat(sizeof(RUNTIME_FUNCTION));

			// Find and update the RUNTIME_FUNCTION in the function. Note: we need to update the
			// offsets inside it, since it might have moved since we used it last.
			RUNTIME_FUNCTION *fn = (RUNTIME_FUNCTION *)(code + ehOffset);
			fn->BeginAddress = startOffset;
			// Note: a bit too late, but it is cumbersome to figure out exactly where.
			fn->EndAddress = startOffset + ehOffset;
			fn->UnwindData = startOffset + dataStart;

			// Find and update the address of the handler function as well:
			Nat exCodeCount = code[dataStart + 2];
			*(DWORD *)(code + dataStart + 4 + roundUp(exCodeCount, Nat(2)))
				= startOffset + Nat(size) - 10;

			return fn;
		}

	}
}

#endif
