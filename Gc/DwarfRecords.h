#pragma once

namespace storm {

	/**
	 * Structures that define the basic structures of DWARF unwind information in order to support
	 * exceptions in the generated code. Assumes that DWARF(2) unwinding data is used in the
	 * compiler. The eh_frame section (which is used during stack unwinding) contains a set of
	 * records. Each record is either a CIE record or a FDE record. CIE records describe general
	 * parameters that can be used from multiple FDE records. FDE records describe what happens in a
	 * single function.
	 *
	 * Note: Examine the file unwind-dw2-fde.h/unwind-dw2-fde.c for details on how the
	 * exception handling is implemented on GCC. We will abuse some structures from there.
	 */

	/**
	 * Configuration:
	 * CHUNK_COUNT - Maximum number of functions inside of each chunk (in the DwarfTable).
	 * CIE_DATA - Maximum number of bytes in the 'data' section of a CIE record.
	 * FDE_DATA - Maximum number of bytes in the 'data' section of a FDE record.
	 */
	static const nat CHUNK_COUNT = 10000; // TODO: Is this a good size?
	static const nat CIE_DATA = 24 - 1;
	static const nat FDE_DATA = 48;

	/**
	 * A generic DWARF record. The fields in here line up so that they match 'CIE' and 'FDE'.
	 */
	struct Record {
		// Length of this record. If it is 0xFFFFFFFF, the next 8 bytes contain a 64-bit
		// length (not supported in this implementation).
		Nat length;

		// Set the length so that this object ends at 'end'.
		void setLen(void *end);
	};

	/**
	 * CIE record.
	 *
	 * A record that contains things that are the same for most functions. Therefore, it is
	 * only stored once per table.
	 */
	struct CIE : Record {
		// Id (zero).
		Nat id;

		// Version.
		Byte version;

		// String containing information followed by the rest of the data.
		Byte data[CIE_DATA];

		// Function declaration used to initialize CIE records.
		typedef void (*InitFn)(CIE *cie);
	};

	/**
	 * FDE record.
	 *
	 * Stores data specific to a function.
	 */
	struct FDE : Record {
		// Offset of the CIE relative to here.
		Nat cieOffset;

		// Data section of the FDE.
		Byte data[FDE_DATA];

		// Set the 'id' field properly.
		void setCie(CIE *to);

		/**
		 * Access the first few members in the FDE. We always use absolute pointers, so we know
		 * their layout!
		 */

		// Start address of the code.
		inline const void *&codeStart() {
			void *x = &data[0];
			return *(const void **)x;
		}
		// Total number of bytes of code.
		inline size_t &codeSize() {
			void *x = &data[8];
			return *(size_t *)x;
		}
		// Size of augmenting data.
		inline Byte &augSize() {
			return data[16];
		}

		// First free offset in 'data'.
		inline Nat firstFree() const {
			return 17;
		}

	};

}
