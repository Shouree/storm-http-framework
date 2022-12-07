#include "stdafx.h"
#include "CodeX64.h"
#include "Gc.h"
#include "Core/GcCode.h"
#include "DwarfTable.h"
#include "Utils/Cache.h"

namespace storm {
	namespace x64 {

		// Will not work properly unless on a 64-bit machine.
#ifdef X64

		static inline bool singleInt(Word value) {
			const Long limit = Long(1) << Long(31);
			Long v(value);
			return v >= -limit && v < limit;
		}

		static inline void writeJump(void *code, const GcCodeRef &ref) {
			// We're dealing with 6 bytes of data. The pointer, located at 'now->offset' and two
			// bytes of op-codes located just before the pointer. For simplicity, we will read 2
			// additional bytes after the pointer so that we can manipulate an entire machine word
			// of 8 bytes when reading/writing to the machine code. This allows us to use atomic
			// instructions properly. We can always do this safely, as the GcCode object will always
			// be allocated directly after the code segment. Thereby, there will always be memory we
			// can legally access (even though we should not trash it).
			//
			// There are two variants of OP-codes used here, short and long. The short variants are
			// used when the target address is within 2GB of the end of the called pointer. The long
			// variant is used otherwise. Both variants need to be the same length, and we can not
			// use a 'nop' instruction to for padding (since then we could alter the instructions
			// after the 'nop' has been executed, but before the actual jump has been executed,
			// causing execution to resume in the middle of an instruction). Therefore, we pad the
			// short variant using a REX.W prefix.
			//
			// The variants are encoded as follows:
			//        short             long
			// call:  48 E8 <offset>    FF 15 <offset>
			// jmp:   48 E9 <offset>    FF 25 <offset>
			//
			// For the short variant, offset is the offset of the actual jump or call target. In the
			// long variant, the offset is the offset to an 8-byte location containing the jump
			// target. We will use this to refer to 'now->pointer'. Both offsets are relative to the
			// last byte of the offset.

			void *mem = ((byte *)code) + ref.offset - 2;
			// Read the current contents of the memory.
			size_t original = unalignedAtomicRead(*(size_t *)mem);

			// Find out if 'jmp' or 'call' was used.
			bool call = false;
			switch (original & 0xFFFF) {
			case 0xE848:
			case 0x15FF:
				call = true;
				break;
			case 0xE948:
			case 0x25FF:
				call = false;
				break;
			default:
				dbg_assert(false, L"Unknown machine code. Jump or call expected!");
				break;
			}

			// Add the uninteresting bytes from the original.
			size_t insert = original & (size_t(0xFFFF) << 48);
			size_t delta = size_t(ref.pointer) - (size_t(mem) + 6);
			if (singleInt(delta)) {
				// Use the short variant.
				insert |= (call ? 0xE848 : 0xE948);
				insert |= (delta & 0xFFFFFFFF) << 16;
			} else {
				// Use the long variant.

				// Compute the offset to '&pointer'.
				delta = size_t(&ref.pointer) - (size_t(mem) + 6);
				insert |= (call ? 0x15FF : 0x25FF);
				insert |= (delta & 0xFFFFFFFF) << 16;
			}

			// Write the value back to memory.
			unalignedAtomicWrite(*(size_t *)mem, insert);
			invalidateICache(mem, (byte *)mem + sizeof(size_t));
		}

#endif

		void writePtr(void *code, const GcCode *refs, Nat id) {
			const GcCodeRef &ref = refs->refs[id];

			switch (ref.kind) {
#ifdef X64
			case GcCodeRef::jump:
				writeJump(code, ref);
				break;
#endif
			case GcCodeRef::unwindInfo:
				if (ref.pointer)
					DwarfChunk::updateFn((FDE *)ref.pointer, code);
				break;
			default:
				dbg_assert(false, L"Only 'jump' is supported by this backend.");
				break;
			}
		}

		void finalize(void *code) {
			GcCode *refs = Gc::codeRefs(code);
			for (size_t i = 0; i < refs->refCount; i++) {
				GcCodeRef &ref = refs->refs[i];
				if (ref.kind == GcCodeRef::unwindInfo && ref.pointer) {
					FDE *ptr = (FDE *)ref.pointer;
					// Set it to null so we do not accidentally scan or free it again.
					atomicWrite(ref.pointer, null);
					dwarfTable().free(ptr);
				}
			}
		}

	}
}
