#pragma once
#include "Core/TObject.h"
#include "Core/Array.h"
#include "Core/GcCode.h"
#include "Reg.h"
#include "Size.h"
#include "Label.h"
#include "Reference.h"

namespace code {
	STORM_PKG(core.asm);

	/**
	 * Binary code output interface. Always outputs in word-order suitable for the current
	 * architecture.
	 */
	class Output : public ObjectOn<Compiler> {
		STORM_CLASS;
	public:
		/**
		 * Low-level output.
		 */

		virtual void STORM_FN putByte(Byte b);  // 1 byte
		virtual void STORM_FN putInt(Nat w);    // 4 bytes
		virtual void STORM_FN putLong(Word w);  // 8 bytes
		virtual void STORM_FN putPtr(Word w);   // 4 or 8 bytes

		// Align the output pointer for the next 'put' operation.
		virtual void STORM_FN align(Nat to);

		/**
		 * Tell the output that we are finished. This is sometimes needed for cache-coherency.
		 */
		virtual void STORM_FN finish();

		/**
		 * Special cases for GC interaction.
		 */

		// Put a custom purpose GC pointer. 'size' bytes.
		virtual void putGc(GcCodeRef::Kind kind, Nat size, Word w);
		void putGc(GcCodeRef::Kind kind, Nat size, Ref ref);

		// Update the last written thing with a custom GC pointer.
		virtual void markGc(GcCodeRef::Kind kind, Nat size, Word w);
		void markGc(GcCodeRef::Kind kind, Nat size, Ref ref);

		// Put a pointer to a gc:d object. 4 or 8 bytes.
		virtual void STORM_FN putGcPtr(Word w);

		// Put a relative pointer to a gc:d object. 4 or 8 bytes.
		virtual void STORM_FN putGcRelative(Word w);

		// Put a relative pointer to a static object (not managed by the Gc). 4 or 8 bytes.
		virtual void STORM_FN putRelativeStatic(Word w);

		// Put an absolute pointer somwhere inside ourself. 4 or 8 bytes.
		virtual void STORM_FN putPtrSelf(Word w);


		// Get the current offset from start.
		virtual Nat STORM_FN tell() const;

		// Write a word with a given size.
		void STORM_FN putSize(Word w, Size size);

		// Labels.
		void STORM_FN mark(Label lbl);
		void STORM_FN mark(MAYBE(Array<Label> *) lbl);
		void STORM_FN putRelative(Label lbl); // Writes 4 bytes.
		void STORM_FN putRelative(Label lbl, Nat offset);
		void STORM_FN putOffset(Label lbl); // Writes 4 bytes. Offset relative to the start of the blob.
		void STORM_FN putAddress(Label lbl); // Writes 4 or 8 bytes.
		Nat STORM_FN offset(Label lbl);

		// References.
		void STORM_FN putRelative(Ref ref); // Writes 4 or 8 bytes.
		void STORM_FN putAddress(Ref ref); // Writes 4 or 8 bytes.
		void putObject(RootObject *obj); // Writes 4 or 8 bytes.

		// Store a (4-byte) reference to a reference or to an object.
		void STORM_FN putObjRelative(Ref ref);
		void putObjRelative(RootObject *obj);

		/**
		 * Call frame information: used during stack unwinding.
		 */

		// Define the base position of the frame (DWARF calls it CFA offset)
		virtual void STORM_FN setFrameOffset(Offset offset);
		// Define the register used for the frame (DWARF calls it the CFA register)
		virtual void STORM_FN setFrameRegister(Reg reg);
		// Define both register and offset for the frame.
		virtual void STORM_FN setFrame(Reg reg, Offset offset);
		// Define that a register has been preserved at 'offset' relative the CFA.
		virtual void STORM_FN markSaved(Reg reg, Offset offset);
		// Define size of a stack frame allocation.
		virtual void STORM_FN markFrameAlloc(Offset size);
		// Mark the end of the prolog.
		virtual void STORM_FN markPrologEnd();

	protected:
		// Mark a label here.
		virtual void STORM_FN markLabel(Nat id);

		// Mark the last added gc-pointer as a reference to something.
		virtual void STORM_FN markGcRef(Ref ref);

		// Get a pointer to the start of the code.
		virtual void *codePtr() const;

		// Find the offset of a label.
		virtual Nat STORM_FN labelOffset(Nat id);

		// Convert an absolute offset to a relative offset.
		virtual Nat STORM_FN toRelative(Nat offset);
	};

	/**
	 * Output variant producing positions for all labels, and the overall size needed by the output.
	 */
	class LabelOutput : public Output {
		STORM_CLASS;
	public:
		STORM_CTOR LabelOutput(Nat ptrSize);

		// Store all label offsets here.
		Array<Nat> *offsets;

		// Total # of bytes needed. Also readable through 'tell'.
		Nat size;

		// # of references needed
		Nat refs;

		virtual void STORM_FN putByte(Byte b);
		virtual void STORM_FN putInt(Nat w);
		virtual void STORM_FN putLong(Word w);
		virtual void STORM_FN putPtr(Word w);
		virtual void STORM_FN align(Nat to);
		virtual void putGc(GcCodeRef::Kind kind, Nat size, Word w);
		virtual void markGc(GcCodeRef::Kind kind, Nat size, Word w);
		virtual void STORM_FN putGcPtr(Word w);
		virtual void STORM_FN putGcRelative(Word w);
		virtual void STORM_FN putRelativeStatic(Word w);
		virtual void STORM_FN putPtrSelf(Word w);

		virtual Nat STORM_FN tell() const;

	protected:
		virtual void STORM_FN markLabel(Nat id);
		virtual void *codePtr() const;
		virtual Nat STORM_FN labelOffset(Nat id);
		virtual Nat STORM_FN toRelative(Nat offset);

	private:
		Nat ptrSize;
		Nat dummy; // For alignment...
	};

	/**
	 * Code output. Provides a pointer to code in the end.
	 */
	class CodeOutput : public Output {
		STORM_CLASS;
	public:
		STORM_CTOR CodeOutput();

		virtual void *codePtr() const;
		virtual void STORM_FN finish();
	};

	/**
	 * Reference which will update a reference in a code segment. Make sure to keep these alive as
	 * long as the code segement is alive somehow.
	 */
	class CodeUpdater : public Reference {
		STORM_CLASS;
	public:
		CodeUpdater(Ref src, Content *inside, void *code, Nat slot);

		// Notification of a new location.
		virtual void moved(const void *newAddr);

	private:
		// The code segment to update.
		UNKNOWN(PTR_GC) void *code;

		// Which slot to update.
		Nat slot;
	};

}
