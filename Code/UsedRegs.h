#pragma once
#include "Core/Object.h"
#include "Core/Array.h"
#include "Reg.h"
#include "Listing.h"

namespace code {
	STORM_PKG(core.asm);

	class Arena;

	/**
	 * Computes and tracks the register usage for instructions in a listing. Each entry tracks the
	 * registers that need to be preserved before each instruction. Ignores ptrStack and ptrFrame.
	 */

	class UsedRegs {
		STORM_VALUE;
	public:
		STORM_CTOR UsedRegs(Array<RegSet *> *used, RegSet *all);

		Array<RegSet *> *used;
		RegSet *all;
	};

	// Computes the used registers which needs to be preserved before each instruction in a
	// listing. Ignores ptrStack and ptrFrame.
	UsedRegs STORM_FN usedRegs(MAYBE(const Arena *) arena, const Listing *src);

	// Computes all used registers, without bothering about per-line representations.
	RegSet *STORM_FN allUsedRegs(const Listing *src);


	namespace impl {

		/**
		 * Work-list used internally when computing used registers.
		 */
		class WorkItem : public Object {
			STORM_CLASS;
		public:
			WorkItem(Nat line);
			WorkItem(Nat line, WorkItem *nextDep);

			// Line to start at.
			Nat line;

			// Inside the work-list?
			Bool inWork;

			// Next item in the dependency list.
			WorkItem *nextDep;

			// Next item in the work list.
			WorkItem *nextWork;
		};

	}
}
