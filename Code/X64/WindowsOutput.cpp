#include "stdafx.h"
#include "WindowsOutput.h"
#include "Gc/CodeTable.h"
#include "Code/Binary.h"
#include "Utils/Bitwise.h"

namespace code {
	namespace x64 {

		WindowsCodeOut::WindowsCodeOut(Binary *owner, Array<Nat> *lbls, Nat size, Nat numRefs) {
			// Properly align 'size'.
			this->size = size = roundUp(size, Nat(sizeof(void *)));

			// Initialize our members.
			this->owner = owner;
			codeRefs = new (this) Array<Reference *>();
			code = (byte *)runtime::allocCode(engine(), size, numRefs + 3);
			labels = lbls;
			pos = 0;
			ref = 3;

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


	}
}
