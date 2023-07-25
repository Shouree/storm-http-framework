#include "stdafx.h"
#include "PosixOutput.h"
#include "Gc/DwarfTable.h"
#include "Code/Binary.h"
#include "Code/Dwarf/Stream.h"
#include "Code/Dwarf/FunctionInfo.h"
#include "DwarfRegisters.h"
#include "Utils/Bitwise.h"

namespace code {
	namespace x64 {

		static const Int codeAlignment = 1;
		static const Nat dataAlignment = -8;

		void initCIE(CIE *cie) {
			Nat pos = code::dwarf::initStormCIE(cie, codeAlignment, dataAlignment, DW_REG_RA);

			code::dwarf::DStream out(cie->data, CIE_DATA, pos);

			// All functions in x86 start with the return pointer pushed on the stack. We emit that
			// once and for all in the CIE record:
			out.putUOp(DW_CFA_def_cfa, DW_REG_RSP, 8); // def_cfa rsp, 8
			out.putUOp(DW_CFA_offset + DW_REG_RA, 1); // offset ra, saved at cfa-8

			assert(!out.overflow(), L"Increase CIE_DATA!");
		}

		PosixCodeOut::PosixCodeOut(Binary *owner, Array<Nat> *lbls, Nat size, Nat numRefs) {
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

			// An entry for the DWARF unwinding information.
			FDE *unwind = dwarfTable().alloc(code, &initCIE);
			fnInfo.set(unwind, codeAlignment, dataAlignment, true, &dwarfRegister);
			refs->refs[2].offset = 0;
			refs->refs[2].kind = GcCodeRef::dwarfInfo;
			refs->refs[2].pointer = unwind;
		}

		void PosixCodeOut::putByte(Byte b) {
			assert(pos < size);
			code[pos++] = b;
		}

		void PosixCodeOut::putInt(Nat w) {
			assert(pos + 3 < size);
			Nat *to = (Nat *)&code[pos];
			*to = w;
			pos += 4;
		}

		void PosixCodeOut::putLong(Word w) {
			assert(pos + 7 < size);
			Word *to = (Word *)&code[pos];
			*to = w;
			pos += 8;
		}

		void PosixCodeOut::putPtr(Word w) {
			assert(pos + 7 < size);
			Word *to = (Word *)&code[pos];
			*to = w;
			pos += 8;
		}

		void PosixCodeOut::align(Nat to) {
			pos = roundUp(pos, to);
		}

		void PosixCodeOut::putGc(GcCodeRef::Kind kind, Nat size, Word w) {
			GcCode *refs = runtime::codeRefs(code);
			assert(ref < refs->refCount);
			refs->refs[ref].offset = pos;
			refs->refs[ref].kind = kind;
			refs->refs[ref].pointer = (void *)w;
			ref++;

			// The actual contents will be updated later...
			pos += size;
		}

		void PosixCodeOut::markGc(GcCodeRef::Kind kind, Nat size, Word w) {
			GcCode *refs = runtime::codeRefs(code);
			assert(ref < refs->refCount);
			assert(pos >= size);
			refs->refs[ref].offset = pos - size;
			refs->refs[ref].kind = kind;
			refs->refs[ref].pointer = (void *)w;
			ref++;
		}

		void PosixCodeOut::putGcPtr(Word w) {
			GcCode *refs = runtime::codeRefs(code);
			assert(ref < refs->refCount);
			refs->refs[ref].offset = pos;
			refs->refs[ref].kind = GcCodeRef::rawPtr;
			refs->refs[ref].pointer = (void *)w;
			ref++;

			putPtr(w);
		}

		void PosixCodeOut::putGcRelative(Word w) {
			GcCode *refs = runtime::codeRefs(code);
			assert(ref < refs->refCount);
			refs->refs[ref].offset = pos;
			refs->refs[ref].kind = GcCodeRef::relativePtr;
			refs->refs[ref].pointer = (void *)w;
			ref++;

			putPtr(0); // Will be updated later...
		}

		void PosixCodeOut::putRelativeStatic(Word w) {
			GcCode *refs = runtime::codeRefs(code);
			assert(ref < refs->refCount);
			refs->refs[ref].offset = pos;
			refs->refs[ref].kind = GcCodeRef::relative;
			refs->refs[ref].pointer = (void *)w;
			ref++;

			putPtr(0); // Will be updated later.
		}

		void PosixCodeOut::putPtrSelf(Word w) {
			GcCode *refs = runtime::codeRefs(code);
			assert(ref < refs->refCount);
			refs->refs[ref].offset = pos;
			refs->refs[ref].kind = GcCodeRef::inside;
			refs->refs[ref].pointer = (void *)(w - Word(codePtr()));
			ref++;

			putPtr(w);
		}

		Nat PosixCodeOut::tell() const {
			return pos;
		}

		void *PosixCodeOut::codePtr() const {
			return code;
		}

		void PosixCodeOut::markLabel(Nat id) {
			// No need. This should already be done for us.
		}

		void PosixCodeOut::markGcRef(Ref r) {
			if (ref == 0)
				return;

			codeRefs->push(new (this) CodeUpdater(r, owner, code, ref - 1));
		}

		Nat PosixCodeOut::labelOffset(Nat id) {
			if (id < labels->count()) {
				return labels->at(id);
			} else {
				assert(false, L"Unknown label id: " + ::toS(id));
				return 0;
			}
		}

		Nat PosixCodeOut::toRelative(Nat offset) {
			return offset - (pos + 4); // NOTE: All relative things on the X86-64 are 4 bytes long, not 8!
		}


	}
}
