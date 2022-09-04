#include "stdafx.h"
#include "Output.h"
#include "Gc/Code.h"
#include "Core/Runtime.h"
#include "Core/GcCode.h"
#include "Utils/Bitwise.h"

namespace code {

	void Output::putByte(Byte b) {
		assert(false);
	}

	void Output::putInt(Nat w) {
		assert(false);
	}

	void Output::putLong(Word w) {
		assert(false);
	}

	void Output::putPtr(Word w) {
		assert(false);
	}

	void Output::align(Nat to) {
		assert(false);
	}

	void Output::putGc(GcCodeRef::Kind kind, Nat size, Word w) {
		assert(false);
	}

	void Output::putGc(GcCodeRef::Kind kind, Nat size, Ref ref) {
		putGc(kind, size, Word(ref.address()));
		markGcRef(ref);
	}

	void Output::markGc(GcCodeRef::Kind kind, Nat size, Word w) {
		assert(false);
	}

	void Output::markGc(GcCodeRef::Kind kind, Nat size, Ref ref) {
		markGc(kind, size, Word(ref.address()));
		markGcRef(ref);
	}

	void Output::putGcPtr(Word w) {
		assert(false);
	}

	void Output::putGcRelative(Word w) {
		assert(false);
	}

	void Output::putRelativeStatic(Word w) {
		assert(false);
	}

	void Output::putPtrSelf(Word w) {
		assert(false);
	}


	Nat Output::tell() const {
		assert(false);
		return 0;
	}

	void Output::putSize(Word w, Size size) {
		if (size == Size::sByte)
			putByte(Byte(w));
		else if (size == Size::sInt)
			putInt(Nat(w));
		else if (size == Size::sLong)
			putLong(w);
		else if (size == Size::sPtr)
			putPtr(w);
		else
			assert(false, L"Unknown size passed to putSize!");
	}

	void Output::mark(Label lbl) {
		markLabel(lbl.id);
	}

	void Output::mark(MAYBE(Array<Label> *) lbl) {
		if (!lbl)
			return;

		for (nat i = 0; i < lbl->count(); i++)
			mark(lbl->at(i));
	}

	void Output::putRelative(Label lbl) {
		putInt(toRelative(labelOffset(lbl.id)));
	}

	void Output::putRelative(Label lbl, Nat offset) {
		putInt(toRelative(labelOffset(lbl.id) + offset));
	}

	void Output::putOffset(Label lbl) {
		putInt(labelOffset(lbl.id));
	}

	void Output::putAddress(Label lbl) {
		Word start = (Word)codePtr();
		Nat offset = labelOffset(lbl.id);
		putPtrSelf(start + offset);
	}

	void Output::putRelative(Ref ref) {
		putGcRelative(Word(ref.address()));
		markGcRef(ref);
	}

	void Output::putAddress(Ref ref) {
		putGcPtr(Word(ref.address()));
		markGcRef(ref);
	}

	void Output::putObject(RootObject *obj) {
		putGcPtr(Word(obj));
	}

	void Output::putObjRelative(Ref ref) {
		putGc(GcCodeRef::relativeHere, sizeof(Int), Word(ref.address()));
		markGcRef(ref);
	}

	void Output::putObjRelative(RootObject *obj) {
		putGc(GcCodeRef::relativeHere, sizeof(Int), Word(obj));
	}

	void Output::markLabel(Nat id) {
		assert(false);
	}

	void Output::markGcRef(Ref ref) {}

	void *Output::codePtr() const {
		assert(false);
		return null;
	}

	Nat Output::labelOffset(Nat id) {
		assert(false);
		return 0;
	}

	Nat Output::toRelative(Nat offset) {
		assert(false);
		return 0;
	}

	void Output::markProlog() {}

	void Output::markEpilog() {}

	void Output::markSaved(Reg reg, Offset offset) {}

	/**
	 * Label output.
	 */

	LabelOutput::LabelOutput(Nat ptrSize) : offsets(new (engine()) Array<Nat>()), ptrSize(ptrSize), size(0), refs(0) {}

	void LabelOutput::putByte(Byte b) {
		size += 1;
	}

	void LabelOutput::putInt(Nat w) {
		size += 4;
	}

	void LabelOutput::putLong(Word w) {
		size += 8;
	}

	void LabelOutput::putPtr(Word w) {
		size += ptrSize;
	}

	void LabelOutput::align(Nat to) {
		size = roundUp(size, to);
	}

	void LabelOutput::putGc(GcCodeRef::Kind kind, Nat size, Word w) {
		this->size += size;
		refs++;
	}

	void LabelOutput::markGc(GcCodeRef::Kind kind, Nat size, Word w) {
		refs++;
	}

	void LabelOutput::putGcPtr(Word w) {
		size += ptrSize;
		refs++;
	}

	void LabelOutput::putGcRelative(Word w) {
		size += ptrSize;
		refs++;
	}

	void LabelOutput::putRelativeStatic(Word w) {
		size += ptrSize;
		refs++;
	}

	void LabelOutput::putPtrSelf(Word w) {
		size += ptrSize;
		refs++;
	}

	Nat LabelOutput::tell() const {
		return size;
	}

	void *LabelOutput::codePtr() const {
		return null;
	}

	void LabelOutput::markLabel(Nat id) {
		while (offsets->count() <= id)
			offsets->push(0);

		offsets->at(id) = tell();
	}

	Nat LabelOutput::labelOffset(Nat id) {
		if (id < offsets->count())
			return offsets->at(id);
		else
			return 0;
	}

	Nat LabelOutput::toRelative(Nat offset) {
		// Irrelevant here.
		return 0;
	}

	/**
	 * Code output.
	 */

	CodeOutput::CodeOutput() {}

	void *CodeOutput::codePtr() const {
		return null;
	}

	/**
	 * Updater.
	 */

	CodeUpdater::CodeUpdater(Ref src, Content *inside, void *code, Nat slot) :
		Reference(src, inside), code(code), slot(slot) {

		moved(address());
	}

	void CodeUpdater::moved(const void *newAddr) {
		GcCode *refs = runtime::codeRefs(code);
		GcCodeRef &ref = refs->refs[slot];

		atomicWrite(ref.pointer, (void *)newAddr);
		storm::gccode::writePtr(code, slot);
	}

}
