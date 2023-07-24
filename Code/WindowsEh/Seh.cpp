#include "stdafx.h"
#include "Seh.h"

#ifdef WINDOWS

#include "Gc/CodeTable.h"

namespace code {
	namespace eh {

#ifdef X64

		RUNTIME_FUNCTION *exceptionCallback(void *pc, void *base) {
			storm::CodeTable &table = storm::codeTable();

			void *found = table.find(pc);
			if (!found)
				return null;

			PLN(L"Found base pointer for function: " << pc << L" at " << found);

			return null;
		}

#endif

	}
}

#endif
