#pragma once
#include "Size.h"
#include "Core/Object.h"
#include "Core/Array.h"

namespace code {
	STORM_PKG(core.asm);

	/**
	 * Registers available for all backends.
	 *
	 * Format: 0xABC where:
	 * A is the size (0 = pointer size)
	 * B is the backend id (0 = general, ...)
	 * C is specific identifier to the backend
	 *
	 * Backend ID:s:
	 * 0 - general (below)
	 * 1, 2 - x86, x64
	 * 3, 4, 5, 6 - arm64
	 */
	enum Reg {
		// No register.
		noReg,

		// Pointer registers:
		ptrStack, // esp
		ptrFrame, // ebp

		ptrA, // return value goes here (eax)
		ptrB, // general purpose, overwritten in function calls
		ptrC, // general purpose, overwritten in function calls

		// 1 byte variants
		al = ptrA | 0x100,
		bl = ptrB | 0x100,
		cl = ptrC | 0x100,

		// 4 byte variants
		eax = ptrA | 0x400,
		ebx = ptrB | 0x400,
		ecx = ptrC | 0x400,

		// 8 byte variants
		rax = ptrA | 0x800,
		rbx = ptrB | 0x800,
		rcx = ptrC | 0x800,
	};

	namespace impl {
		// TODO: Remove these when we properly handle enums!
		inline Reg STORM_FN noReg() { return code::noReg; }
		inline Reg STORM_FN ptrStack() { return code::ptrStack; }
		inline Reg STORM_FN ptrFrame() { return code::ptrFrame; }
		inline Reg STORM_FN ptrA() { return code::ptrA; }
		inline Reg STORM_FN ptrB() { return code::ptrB; }
		inline Reg STORM_FN ptrC() { return code::ptrC; }
		inline Reg STORM_FN al() { return code::al; }
		inline Reg STORM_FN bl() { return code::bl; }
		inline Reg STORM_FN cl() { return code::cl; }
		inline Reg STORM_FN eax() { return code::eax; }
		inline Reg STORM_FN ebx() { return code::ebx; }
		inline Reg STORM_FN ecx() { return code::ecx; }
		inline Reg STORM_FN rax() { return code::rax; }
		inline Reg STORM_FN rbx() { return code::rbx; }
		inline Reg STORM_FN rcx() { return code::rcx; }
	}

	// Get the name of a register.
	const wchar *name(Reg r);

	// Size of registers.
	Size STORM_FN size(Reg r);

	// Get the corresponding register with another size.
	Reg STORM_FN asSize(Reg r, Size size);

	// Are the two registers the same, disregarding size?
	Bool STORM_FN same(Reg a, Reg b);

	// Find a free register out of the three registers that are usable by default. Always returns a
	// pointer-sized register.
	Reg STORM_FN freeReg(Reg a);
	Reg STORM_FN freeReg(Reg a, Reg b);

	/**
	 * Set of registers. Considers registers of different sizes to be the same, but keeps track of
	 * the largest integer sized register used. Byte-sizes are promoted to 32-bit sizes.
	 *
	 * ptrStack and ptrBase are not considered at all.
	 */
	class RegSet : public Object {
		STORM_CLASS;
	public:
		// Create with no registers.
		STORM_CTOR RegSet();

		// Copy.
		RegSet(const RegSet &src);

		// Create with a single registers.
		STORM_CTOR RegSet(Reg r);

		// Array of registers -> set.
		STORM_CTOR RegSet(Array<Reg> *regs);

		// Deep copy.
		void STORM_FN deepCopy(CloneEnv *env);

		// Contains a specific register?
		Bool STORM_FN has(Reg r) const;

		// Add registers.
		void STORM_FN put(Reg r);
		void STORM_FN put(const RegSet *r);

		// Get the largest register seen.
		Reg STORM_FN get(Reg r) const;

		// Get the number of registers in here.
		inline Nat STORM_FN count() const { return numSet; }

		// Remove register.
		void STORM_FN remove(Reg r);

		// Clear.
		void STORM_FN clear();

		// Get all registers in here.
		// Array<Reg> *STORM_FN all() const;

		// Iterator.
		class Iter {
			STORM_VALUE;
		public:
			Iter();
			Iter(const RegSet *reg);

			Bool STORM_FN operator ==(Iter o) const;
			Bool STORM_FN operator !=(Iter o) const;

			Iter &STORM_FN operator ++();
			Iter STORM_FN operator ++(int z);

			Reg operator *() const;
			Reg STORM_FN v() const;

		private:
			const RegSet *owner;

			// Pos: lower 4 bits = slot, rest = bank.
			Nat pos;

			// At end?
			bool atEnd() const;

			// Empty at pos?
			bool empty(Nat pos) const;
			Reg read(Nat pos) const;
		};

		Iter STORM_FN begin() const;
		Iter STORM_FN end() const;

		// ToS.
		virtual void STORM_FN toS(StrBuf *to) const;

	private:
		/**
		 * Data storage. We store 'banks'*16 entries here as follows:
		 *
		 * 'index' stores the backed id for the 'dataX' segments. 'data0' always has backend id = 0,
		 * as those are commonly used. 'index' stores four bits for each 'dataX', zero means free.
		 *
		 * Each of 'dataX' stores two bits for each entry inside it. Each entry represents one
		 * possible value of the last four bits of a Reg. These two bits have the following
		 * four values:
		 *
		 * 0: not in the set
		 * 1: 32 bit value is in the set
		 * 2: pointer is in the set
		 * 3: 64 bit value is in the set
		 *
		 * We only need this few registers as it is uncommon to mix registers from different
		 * backends. If some backends require larger storage in the future, this scheme is easily
		 * expandable.
		 */

		// Constants:
		enum {
			// 5 banks is enough for now. Make sure to add 'data' entries up to the number of banks used.
			// At least 2, maximum 9.
			banks = 5,

			// Slots per data entry. Should be 16.
			dataSlots = 8 * sizeof(Nat) / 2,
		};

		// Index. Note that data0 is excluded from the index as that always represents the common
		// registers above.
		Nat index;

		// Data. Make sure to have at least 'banks' of these.
		Nat data0;
		Nat data1;
		Nat data2;
		Nat data3;
		Nat data4;

		// # of registers in here.
		Nat numSet;

		/**
		 * Low-level access to the data. An id points into one two-bit position inside data (with an
		 * associated index location).
		 */

		// Read/write index.
		Nat readIndex(Nat bank) const;
		void writeIndex(Nat bank, Nat v);

		// Read/write data.
		Nat readData(Nat bank, Nat slot) const;
		void writeData(Nat bank, Nat slot, Nat v);

		// Bank manipulation.
		bool emptyBank(Nat bank) const;
		Nat findBank(Nat backendId) const;
		Nat allocBank(Nat backendId);

		// Read a register.
		Reg readRegister(Nat bank, Nat slot) const;

		/**
		 * Register manipulation helpers.
		 */

		// Get the slot id of a register.
		static Nat registerSlot(Reg r);
		static Nat registerBackend(Reg r);
		static Nat registerSize(Reg r);

	};

}
