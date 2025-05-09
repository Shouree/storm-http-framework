#pragma once
#include "Code/Output.h"
#include "Core/GcCode.h"

namespace code {
	class Binary;
	namespace x64 {
		STORM_PKG(core.asm.x64);

		/**
		 * Version of the LabelOutput class that keeps track of the size of the exception metadata.
		 */
		class WindowsLabelOut : public LabelOutput {
			STORM_CLASS;
		public:
			STORM_CTOR WindowsLabelOut();

			// Number of slots required for unwind codes.
			Nat unwindCount;

			// Length of prolog in bytes.
			Nat prologSize;

			// Additional members for the metadata we care about:
			virtual void STORM_FN markSaved(Reg reg, Offset offset);
			virtual void STORM_FN markFrameAlloc(Offset size);
			virtual void STORM_FN markPrologEnd();
		};


		/**
		 * Output class specific to Windows. Stores Windows-specific exception handling data.
		 */
		class WindowsCodeOut : public CodeOutput {
			STORM_CLASS;
		public:
			STORM_CTOR WindowsCodeOut(Binary *owner, WindowsLabelOut *size);

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

			virtual void *codePtr() const;

			virtual void STORM_FN markSaved(Reg reg, Offset offset);
			virtual void STORM_FN markFrameAlloc(Offset size);

		protected:
			virtual void STORM_FN markLabel(Nat id);
			virtual void STORM_FN markGcRef(Ref ref);
			virtual Nat STORM_FN labelOffset(Nat id);
			virtual Nat STORM_FN toRelative(Nat id);

		private:
			// Our owner. Used to create proper references.
			Binary *owner;

			// Updaters in the code.
			Array<Reference *> *codeRefs;

			// Code we're writing to.
			UNKNOWN(PTR_GC) byte *code;

			// Position in the code.
			Nat pos;

			// Size of the code.
			Nat size;

			// First free ref.
			Nat ref;

			// Position inside the metadata (byte offsets).
			Nat metaPos;

			// Label offsets, computed from before.
			Array<Nat> *labels;
		};

	}
}
