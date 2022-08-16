#include "stdafx.h"
#include "Output.h"
#include "Gc/DwarfTable.h"
#include "../Binary.h"
#include "Utils/Bitwise.h"

namespace code {
	namespace arm64 {

		CodeOut::CodeOut(Binary *owner, Array<Nat> *lbls, Nat size, Nat numRefs) {
			// Properly align 'size'.
			this->size = size = roundUp(size, Nat(sizeof(void *)));

			// Initialize our members.
			this->owner = owner;
			codeRefs = new (this) Array<Reference *>();
			code = (byte *)runtime::allocCode(engine(), size + sizeof(void *), numRefs + 2);
			labels = lbls;
			pos = 0;
			ref = 2;

			GcCode *refs = runtime::codeRefs(code);

			// An entry for the DWARF unwinding information.
			FDE *unwind = storm::dwarfTable().alloc(code, &code::x64::initDwarfCIE);
			fnInfo.set(unwind);
			refs->refs[0].offset = 0;
			refs->refs[0].kind = GcCodeRef::unwindInfo;
			refs->refs[0].pointer = unwind;

			// Store 'codeRefs' at the end of our allocated space.
			refs->refs[1].offset = size;
			refs->refs[1].kind = GcCodeRef::rawPtr;
			refs->refs[1].pointer = codeRefs;
		}

		void CodeOut::putByte(Byte b) {
			assert(pos < size);
			code[pos++] = b;
		}

		void CodeOut::putInt(Nat w) {
			assert(pos + 3 < size);
			Nat *to = (Nat *)&code[pos];
			*to = w;
			pos += 4;
		}

		void CodeOut::putLong(Word w) {
			assert(pos + 7 < size);
			Word *to = (Word *)&code[pos];
			*to = w;
			pos += 8;
		}

		void CodeOut::putPtr(Word w) {
			assert(pos + 7 < size);
			Word *to = (Word *)&code[pos];
			*to = w;
			pos += 8;
		}

		void CodeOut::align(Nat to) {
			pos = roundUp(pos, to);
		}

		void CodeOut::putGc(GcCodeRef::Kind kind, Nat size, Word w) {
			GcCode *refs = runtime::codeRefs(code);
			assert(ref < refs->refCount);
			refs->refs[ref].offset = pos;
			refs->refs[ref].kind = kind;
			refs->refs[ref].pointer = (void *)w;
			ref++;

			// The actual contents will be updated later...
			pos += size;
		}

		void CodeOut::putGcPtr(Word w) {
			GcCode *refs = runtime::codeRefs(code);
			assert(ref < refs->refCount);
			refs->refs[ref].offset = pos;
			refs->refs[ref].kind = GcCodeRef::rawPtr;
			refs->refs[ref].pointer = (void *)w;
			ref++;

			putPtr(w);
		}

		void CodeOut::putGcRelative(Word w) {
			GcCode *refs = runtime::codeRefs(code);
			assert(ref < refs->refCount);
			refs->refs[ref].offset = pos;
			refs->refs[ref].kind = GcCodeRef::relativePtr;
			refs->refs[ref].pointer = (void *)w;
			ref++;

			putPtr(0); // Will be updated later...
		}

		void CodeOut::putRelativeStatic(Word w) {
			GcCode *refs = runtime::codeRefs(code);
			assert(ref < refs->refCount);
			refs->refs[ref].offset = pos;
			refs->refs[ref].kind = GcCodeRef::relative;
			refs->refs[ref].pointer = (void *)w;
			ref++;

			putPtr(0); // Will be updated later.
		}

		void CodeOut::putPtrSelf(Word w) {
			GcCode *refs = runtime::codeRefs(code);
			assert(ref < refs->refCount);
			refs->refs[ref].offset = pos;
			refs->refs[ref].kind = GcCodeRef::inside;
			refs->refs[ref].pointer = (void *)(w - Word(codePtr()));
			ref++;

			putPtr(w);
		}

		Nat CodeOut::tell() const {
			return pos;
		}

		void *CodeOut::codePtr() const {
			return code;
		}

		void CodeOut::markLabel(Nat id) {
			// No need. This should already be done for us.
		}

		void CodeOut::markGcRef(Ref r) {
			if (ref == 0)
				return;

			codeRefs->push(new (this) CodeUpdater(r, owner, code, ref - 1));
		}

		Nat CodeOut::labelOffset(Nat id) {
			if (id < labels->count()) {
				return labels->at(id);
			} else {
				assert(false, L"Unknown label id: " + ::toS(id));
				return 0;
			}
		}

		Nat CodeOut::toRelative(Nat offset) {
			return offset - (pos + 4); // NOTE: All relative things on the X86-64 are 4 bytes long, not 8!
		}

		void CodeOut::markProlog() {
			fnInfo.prolog(pos);
		}

		void CodeOut::markEpilog() {
			fnInfo.epilog(pos);
		}

		void CodeOut::markSaved(Reg reg, Offset offset) {
			fnInfo.preserve(pos, reg, offset);
		}


	}
}
