#include "stdafx.h"
#include "CodeArm64.h"
#include "Gc.h"
#include "Core/GcCode.h"
#include "DwarfTable.h"

namespace storm {
	namespace arm64 {

		static inline bool imm26(Nat value) {
			const Int limit = Nat(1) << 25;
			Int v = value;
			return v >= -limit && v < limit;
		}

		// Detect branch with link (cf. branch without link). Assumes the op-code is one of "bl",
		// "blr", "b", or "br".
		static inline bool isBranchLink(Nat op) {
			if (((op >> 26) & 0x1F) == 0x05) {
				// BL or B, look at MSB.
				return (op >> 31) != 0;
			} else {
				// BLR or BR, look at bit 21.
				return ((op >> 21) & 0x1) != 0;
			}
		}

		void writePtr(void *code, const GcCode *refs, Nat id) {
			const GcCodeRef &ref = refs->refs[id];
			void *write;
			Nat original = 0;
			Nat delta = 0;

			switch (ref.kind) {
			case GcCodeRef::jump:
				// For jumps, we store a load(imm) into x17 first, followed by a call. That way, we
				// can change the call instruction between an immediate version, and a "long"
				// version depending on the current distance. We need to leave the load instruction
				// so that we may switch at any time.
				write = ((byte *)code) + ref.offset + sizeof(Nat);
				delta = size_t(ref.pointer) - size_t(write);
				delta /= 4; // 2 zero bits are implicit

				original = *(Nat *)write;

				if (isBranchLink(original)) {
					if (imm26(delta)) {
						// BL
						original = 0x94000000 | (0x03FFFFFF & delta);
					} else {
						// BLR x17
						original = 0xD63F0220;
					}
				} else {
					if (imm26(delta)) {
						// B
						original = 0x14000000 | (0x03FFFFFF & delta);
					} else {
						// BR x17
						original = 0xD61F0220;
					}
				}

				shortUnalignedAtomicWrite(*(Nat *)write, original);

				// Fall through to update the load instruction.
				// fall through
			case GcCodeRef::relativeHereImm19:
				write = ((byte *)code) + ref.offset;
				delta = size_t(&ref.pointer) - size_t(write);
				delta /= 4; // 2 zero bits are implicit

				original = *(Nat *)write;
				original &= 0xFF00001F;
				original |= (delta & 0x7FFFF) << 5;
				shortUnalignedAtomicWrite(*(Nat *)write, original);
				break;
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
