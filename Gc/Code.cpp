#include "stdafx.h"
#include "Code.h"
#include "Gc.h"
#include "Core/GcCode.h"

#include "CodeTable.h"
#include "DwarfTable.h"

#include "CodeX86.h"
#include "CodeX64.h"
#include "CodeArm64.h"

#if defined(X86)
#define ARCH x86
#elif defined(X64)
#define ARCH x64
#elif defined(ARM64)
#define ARCH arm64
#else
#error "Updating code segments is not implemented for your architecture yet!"
#endif

namespace storm {
	namespace gccode {

		// The generic cases. These are implemented the same on every platform.
		static void doWritePtr(void *code, const GcCode *refs, Nat id) {
			const GcCodeRef &ref = refs->refs[id];
			size_t ptr = size_t(ref.pointer);
			void *write = ((byte *)code) + ref.offset;

			// Note: These writes need to be visible as an atomic operation to the instruction
			// decoding unit of the CPU. Further atomicity is not required, as the GC arranges for
			// that anyway. Offsets might not be aligned as regular pointers on the system.

			switch (ref.kind) {
			case GcCodeRef::disabled:
			case GcCodeRef::ptrStorage:
				// Nothing to do...
				break;
			case GcCodeRef::rawPtr:
				unalignedAtomicWrite(*(size_t *)write, ptr);
				invalidateSingleICache(write);
				break;
			case GcCodeRef::relativePtr:
			case GcCodeRef::relative:
				unalignedAtomicWrite(*(size_t *)write, ptr - (size_t(write) + sizeof(size_t)));
				invalidateSingleICache(write);
				break;
			case GcCodeRef::inside:
				unalignedAtomicWrite(*(size_t *)write, ptr + size_t(code));
				invalidateSingleICache(write);
				break;
			case GcCodeRef::relativeHere:
				// Write a relative pointer to 'pointer' itself. Should always be the same, but the
				// offset is not really exposed conveniently anywhere else.
				ptr = size_t(&ref.pointer);
				shortUnalignedAtomicWrite(*(nat *)write, nat(ptr - (size_t(write) + sizeof(nat))));
				invalidateSingleICache(write);
				break;
			case GcCodeRef::codeInfo:
				if (ref.pointer)
					CodeTable::update(ref.pointer, code);
				break;
			case GcCodeRef::dwarfInfo:
				if (ref.pointer)
					DwarfChunk::updateFn((FDE *)ref.pointer, code);
				break;
			default:
				// Pass on to the architecture specific parts:
				ARCH::writePtr(code, refs, id);
				break;
			}
		}

		void updatePtrs(void *code, const GcCode *refs) {
			for (Nat i = 0; i < refs->refCount; i++)
				doWritePtr(code, refs, i);
		}

		void writePtr(void *code, Nat id) {
			GcCode *refs = Gc::codeRefs(code);
			doWritePtr(code, refs, id);
		}

		Bool needFinalization() {
			return ARCH::needFinalization();
		}

		void finalize(void *code) {
			GcCode *refs = Gc::codeRefs(code);
			for (size_t i = 0; i < refs->refCount; i++) {
				GcCodeRef &ref = refs->refs[i];
				if (ref.pointer) {
					if (ref.kind == GcCodeRef::codeInfo) {
						CodeTable::Handle h = ref.pointer;

						atomicWrite(ref.pointer, null);
						codeTable().remove(h);
					} else if (ref.kind == GcCodeRef::dwarfInfo) {
						FDE *ptr = (FDE *)ref.pointer;

						atomicWrite(ref.pointer, null);
						dwarfTable().free(ptr);
					}
				}
			}

			ARCH::finalize(code);
		}

	}
}

