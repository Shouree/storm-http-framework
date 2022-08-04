#include "stdafx.h"
#include "CodeArm64.h"
#include "Gc.h"
#include "Core/GcCode.h"
#include "DwarfTable.h"

namespace storm {
	namespace arm64 {

		void writePtr(void *code, const GcCode *refs, Nat id) {
			const GcCodeRef &ref = refs->refs[id];

			switch (ref.kind) {
				// TODO!
			case GcCodeRef::unwindInfo:
				if (ref.pointer)
					DwarfChunk::updateFn((FDE *)ref.pointer, code);
				break;
			default:
				dbg_assert(false, L"Unsupported reference type!");
				break;
			}
		}

		void finalize(void *code) {
			GcCode *refs = Gc::codeRefs(code);
			for (size_t i = 0; i < refs->refCount; i++) {
				GcCodeRef &ref = refs->refs[i];
				if (ref.kind == GcCodeRef::unwindInfo && ref.pointer) {
					FDE *ptr = (FDE *)ref.pointer;
					// Set it to null so we do no accidentally scan or free it again.
					atomicWrite(ref.pointer, null);
					dwarfTable().free(ptr);
				}
			}
		}

	}
}
