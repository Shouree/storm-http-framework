#include "stdafx.h"
#include "WindowsOutput.h"
#include "Gc/CodeTable.h"
#include "Code/Binary.h"
#include "Code/WindowsEh/Seh.h"
#include "Code/WindowsEh/Seh64.h"
#include "Utils/Bitwise.h"

namespace code {
	namespace x64 {

		using namespace code::eh;

		/**
		 * Size computation.
		 */

		WindowsLabelOut::WindowsLabelOut() : LabelOutput(8), unwindCount(0) {}

		void WindowsLabelOut::markSaved(Reg reg, Offset offset) {
			assert(offset == Offset(), L"Only push-type saves are supported on Windows.");
			// Note: There is an op-code for save, so we could do something similar to the Posix
			// version if we want to. However, it would require careful construction of epilogs.
			unwindCount += 1;
		}

		void WindowsLabelOut::markFrameAlloc(Offset size) {
			Nat sz = size.v64();
			if (sz == 0)
				;
			else if (sz < 128)
				unwindCount += 1;
			else if (sz < 512 * 1024)
				unwindCount += 2;
			else
				unwindCount += 3;
		}

		void WindowsLabelOut::markPrologEnd() {
			prologSize = size;
		}



		/**
		 * Code output.
		 */

		WindowsCodeOut::WindowsCodeOut(Binary *owner, WindowsLabelOut *out) {
			// Compute size, round up to proper alignment:
			size = roundUp(out->size, Nat(4));

			// Then add size of unwind metadata:
			Nat metaStart = size;
			size += Nat(sizeof(RuntimeFunction));
			size += Nat(sizeof(UnwindInfo));
			size += 2 * roundUp(out->unwindCount, Nat(2)); // Aligned unwind words.
			size += Nat(sizeof(Nat)); // Exception handler address.
			size += 6; // JMP instruction to the real exception handler.
			size += Nat(sizeof(Nat)); // Size of the metadata, so that we can find the previous chunk.
			size = roundUp(size, Nat(sizeof(void *)));

			// Initialize our members.
			this->owner = owner;
			codeRefs = new (this) Array<Reference *>();
			code = (byte *)runtime::allocCode(engine(), size, out->refs + 4);
			labels = out->offsets;
			pos = 0;
			ref = 4;

			GcCode *refs = runtime::codeRefs(code);

			// Store a reference to the binary in the first element of the blob.
			refs->refs[0].offset = 0;
			refs->refs[0].kind = GcCodeRef::ptrStorage;
			refs->refs[0].pointer = owner;

			// Store 'codeRefs' also. We need to keep it alive.
			refs->refs[1].offset = 0;
			refs->refs[1].kind = GcCodeRef::ptrStorage;
			refs->refs[1].pointer = codeRefs;

			// An entry for the code table.
			CodeTable::Handle table = codeTable().add(code);
			refs->refs[2].offset = 0;
			refs->refs[2].kind = GcCodeRef::codeInfo;
			refs->refs[2].pointer = table;

			/**
			 * Initialize metadata table:
			 */

			// Store offset to the start, so that we can find it later:
			*(Nat *)&code[size - 4] = metaStart;

			// Store the jump instruction just before:
			code[size - 10] = 0xFF;
			code[size - 9] = 0x25;
			refs->refs[3].offset = size - 8;
			refs->refs[3].kind = GcCodeRef::jump;
			refs->refs[3].pointer = (void *)address(code::eh::windowsHandler);

			assert(out->prologSize < 256, L"Too long prolog. This is a bug in the backend.");

			// Initialize the unwind information:
			UnwindInfo *uwInfo = (UnwindInfo *)&code[metaStart + sizeof(RuntimeFunction)];
			uwInfo->version = 1;
			uwInfo->flags = UnwindFlagExamine | UnwindFlagUnwind; // TODO: specify which we actually need?
			uwInfo->prologSize = out->prologSize;
			uwInfo->unwindCount = out->unwindCount;
			uwInfo->frameRegister = 0;
			uwInfo->frameOffset = 0;

			metaPos = metaStart + Nat(sizeof(RuntimeFunction) + sizeof(UnwindInfo));
			metaPos += out->unwindCount * 2; // We need to fill the unwind data backwards.
		}

		void WindowsCodeOut::putByte(Byte b) {
			assert(pos < size);
			code[pos++] = b;
		}

		void WindowsCodeOut::putInt(Nat w) {
			assert(pos + 3 < size);
			Nat *to = (Nat *)&code[pos];
			*to = w;
			pos += 4;
		}

		void WindowsCodeOut::putLong(Word w) {
			assert(pos + 7 < size);
			Word *to = (Word *)&code[pos];
			*to = w;
			pos += 8;
		}

		void WindowsCodeOut::putPtr(Word w) {
			assert(pos + 7 < size);
			Word *to = (Word *)&code[pos];
			*to = w;
			pos += 8;
		}

		void WindowsCodeOut::align(Nat to) {
			pos = roundUp(pos, to);
		}

		void WindowsCodeOut::putGc(GcCodeRef::Kind kind, Nat size, Word w) {
			GcCode *refs = runtime::codeRefs(code);
			assert(ref < refs->refCount);
			refs->refs[ref].offset = pos;
			refs->refs[ref].kind = kind;
			refs->refs[ref].pointer = (void *)w;
			ref++;

			// The actual contents will be updated later...
			pos += size;
		}

		void WindowsCodeOut::markGc(GcCodeRef::Kind kind, Nat size, Word w) {
			GcCode *refs = runtime::codeRefs(code);
			assert(ref < refs->refCount);
			assert(pos >= size);
			refs->refs[ref].offset = pos - size;
			refs->refs[ref].kind = kind;
			refs->refs[ref].pointer = (void *)w;
			ref++;
		}

		void WindowsCodeOut::putGcPtr(Word w) {
			GcCode *refs = runtime::codeRefs(code);
			assert(ref < refs->refCount);
			refs->refs[ref].offset = pos;
			refs->refs[ref].kind = GcCodeRef::rawPtr;
			refs->refs[ref].pointer = (void *)w;
			ref++;

			putPtr(w);
		}

		void WindowsCodeOut::putGcRelative(Word w) {
			GcCode *refs = runtime::codeRefs(code);
			assert(ref < refs->refCount);
			refs->refs[ref].offset = pos;
			refs->refs[ref].kind = GcCodeRef::relativePtr;
			refs->refs[ref].pointer = (void *)w;
			ref++;

			putPtr(0); // Will be updated later...
		}

		void WindowsCodeOut::putRelativeStatic(Word w) {
			GcCode *refs = runtime::codeRefs(code);
			assert(ref < refs->refCount);
			refs->refs[ref].offset = pos;
			refs->refs[ref].kind = GcCodeRef::relative;
			refs->refs[ref].pointer = (void *)w;
			ref++;

			putPtr(0); // Will be updated later.
		}

		void WindowsCodeOut::putPtrSelf(Word w) {
			GcCode *refs = runtime::codeRefs(code);
			assert(ref < refs->refCount);
			refs->refs[ref].offset = pos;
			refs->refs[ref].kind = GcCodeRef::inside;
			refs->refs[ref].pointer = (void *)(w - Word(codePtr()));
			ref++;

			putPtr(w);
		}

		Nat WindowsCodeOut::tell() const {
			return pos;
		}

		void *WindowsCodeOut::codePtr() const {
			return code;
		}

		void WindowsCodeOut::markLabel(Nat id) {
			// No need. This should already be done for us.
		}

		void WindowsCodeOut::markGcRef(Ref r) {
			if (ref == 0)
				return;

			codeRefs->push(new (this) CodeUpdater(r, owner, code, ref - 1));
		}

		Nat WindowsCodeOut::labelOffset(Nat id) {
			if (id < labels->count()) {
				return labels->at(id);
			} else {
				assert(false, L"Unknown label id: " + ::toS(id));
				return 0;
			}
		}

		Nat WindowsCodeOut::toRelative(Nat offset) {
			return offset - (pos + 4); // NOTE: All relative things on the X86-64 are 4 bytes long, not 8!
		}

		void WindowsCodeOut::markSaved(Reg reg, Offset offset) {
			// TODO: Handle XMM registers? I don't think we ever spill them...
			metaPos -= 2;
			code[metaPos + 0] = pos;
			code[metaPos + 1] = UnwindPushNonvol | (win64Register(reg) << 4);
		}

		void WindowsCodeOut::markFrameAlloc(Offset size) {
			Nat sz = size.v64();
			assert(sz % 8 == 0, L"Invalid stack allocation size. This is a bug in the backend.");
			if (sz == 0) {
				// Nothing to do.
			} else if (sz < 128) {
				sz = (sz - 8) / 8; // Scale according to docs.
				metaPos -= 2;
				code[metaPos + 0] = pos;
				code[metaPos + 1] = UnwindAllocSmall | (sz << 4);
			} else if (sz < 512 * 1024) {
				sz = sz / 8; // Scale according to docs.
				metaPos -= 4;
				code[metaPos + 0] = pos;
				code[metaPos + 1] = UnwindAllocLarge | (0 << 4);
				// Second short: store in little endian
				code[metaPos + 2] = sz & 0xFF;
				code[metaPos + 3] = (sz >> 8) & 0xFF;
			} else {
				metaPos -= 6;
				code[metaPos + 0] = pos;
				code[metaPos + 1] = UnwindAllocLarge | (1 << 4);
				// Store a 32-bit integer containing the total size. Note: might not be aligned
				// properly, so we encode it manually.
				// Note: We *don't* have to divide size by 8 here!
				code[metaPos + 2] = sz & 0xFF;
				code[metaPos + 3] = (sz >> 8) & 0xFF;
				code[metaPos + 4] = (sz >> 16) & 0xFF;
				code[metaPos + 5] = (sz >> 24) & 0xFF;
			}
		}

	}
}
