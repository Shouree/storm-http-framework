#include "stdafx.h"
#include "CodeX86.h"
#include "Gc.h"
#include "Core/GcCode.h"
#include "CodeTable.h"

namespace storm {
	namespace x86 {

		void writePtr(void *code, const GcCode *refs, Nat id) {
			dbg_assert(false, L"Unknown pointer type.");
			// const GcCodeRef &ref = refs->refs[id];
			// switch (ref.kind) {
			// default:
			// 	dbg_assert(false, L"Unknown pointer type.");
			// 	break;
			// }
		}

		void finalize(void *code) {
			(void)code;
		}

	}
}
