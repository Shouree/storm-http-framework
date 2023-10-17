#pragma once
#include "Core/TObject.h"
#include "Core/GcType.h"
#include "Arena.h"
#include "Listing.h"
#include "RefSource.h"
#include "StackFrame.h"

namespace code {
	STORM_PKG(core.asm);

	/**
	 * A Binary represents a Listing that has been translated into machine code, along with any
	 * extra information needed (such as descriptions of exception handlers or similar).
	 *
	 * Note: The Binary class expects backends to create a table of data starting at the meta() label
	 * where each element looks like this:
	 *   size_t freeFn; - a pointer to the function to be executed when this variable is to be freed.
	 *   size_t offset; - a signed offset to the variable relative to the stack frame provided by the backend.
	 * There shall be one entry for each variable in the Listing. Variable 0 shall be at index 0.
	 */
	class Binary : public Content {
		STORM_CLASS;
	public:
		// Translate a listing into machine code.
		STORM_CTOR Binary(Arena *arena, Listing *src);

		// Output the transformed ASM code for debugging.
		Binary(Arena *arena, Listing *src, Bool debug);

		// Clean up a stack frame from this function.
		void cleanup(StackFrame &frame);

		// Clean up a stack fraome from this function from the current block, up to and including
		// 'until'. Returns the parent of 'until', which is now to be considered the current part.
		Nat cleanup(StackFrame &frame, Nat until);

		// Check if we have any catch-clauses at all (used for pre-screening in exception handlers).
		inline bool hasCatch() const { return tryBlocks != null; }

		// Information on how to resume from a try-clause.
		struct Resume {
			// Next instruction to execute. 'null' if nothing is to be done.
			void *ip;

			// Offset from the frame pointer to the stack pointer.
			ptrdiff_t stackOffset;

			// Part outside the one handling the exception.
			size_t cleanUntil;
		};

		// Check if we have a catch-clause for the active part or any of its parents.
		bool hasCatch(Nat part, RootObject *exception, Resume &resume);

		// Get the stack offset for this binary, i.e. the difference between the stack pointer and
		// the frame pointer for this function (mainly used for cleanup during exceptions).
		ptrdiff_t stackOffset() const;

		// Output to string.
		virtual void STORM_FN toS(StrBuf *to) const;

	private:
		// Information about a single variable.
		struct Variable {
			Nat id;

			// The low 16 bits contain FreeOpt, the high 16 bits contain the size of the variable.
			Nat flags;

			enum {
				sByte = 0x1000,
				sInt = 0x2000,
				sLong = 0x3000,
				sPtr = 0x4000,
				sMask = 0xFF000
			};
		};

		// Remember the block hierarchy. These are allocated as GcArray:s to reduce the number of allocations.
		struct Block {
			// Number of elements.
			const size_t count;

			// Parent block.
			size_t parent;

			// Variables in here.
			Variable vars[1];
		};

		// Information for try-blocks.
		struct TryInfo {
			// Current block id.
			Nat blockId;

			// Offset to continue execution from.
			Nat resumeOffset;

			// Type to catch.
			Type *type;
		};

		// Offset of the metadata label.
		Nat metaOffset;

		// All parts.
		GcArray<Block *> *blocks;

		// Try-block information. 'null' if no exceptions are caught. Indices do not correspond to
		// parts here since the array is sparse. It is necessary to perform search the array
		// (perhaps using a binary search).
		GcArray<TryInfo> *tryBlocks;

		// Compile the Listing object.
		void compile(Arena *arena, Listing *src, Bool debug);

		// Fill the 'blocks' array.
		void fillBlocks(Listing *src);

		// Fill the 'tryBlocks' array if necessary.
		void fillTryBlocks(Listing *src, LabelOutput *labels);

		// Clean a single variable.
		void cleanup(StackFrame &frame, Variable &v);

		// Type declarations for the GC.
		static const GcType blockArrayType;
		static const GcType blockType;
		static const GcType tryInfoArrayType;
	};

}
