#pragma once

namespace storm {

	/**
	 * Describes a reference inside the generated code.
	 *
	 * The actual pointers for the references are stored inside the 'GcCodeRef' itself. These are
	 * the pointers that are actually scanned by the garbage collector, but the garbage collector
	 * will make sure to keep the described reference updated as well. Call
	 * 'runtime::codeUpdatePtrs' when all references have been filled in to update the references
	 * which 'offset' denotes.
	 *
	 * In the case of 'inside' references, the value stored in the 'pointer' member is a value to be
	 * interpreted as the offset within the current object, rather than an actual pointer.
	 *
	 * Note: If the LSB of the kind is set, the 'pointer' for that entry is scanned by the GC. To
	 * simplify, we use the entire least significant hex digit to indicate if the entry is scanned
	 * or not. The rest of the numbers are increasing as the enum would do normally.
	 */
	struct GcCodeRef {
		// Kind of reference. Ie. how do we read or write this reference to/from memory?
		enum Kind {
			// Disabled for now.
			disabled = 0x00,

			// Raw pointer. Reads 4/8 bytes from 'offset' and treats those as a pointer.
			rawPtr = 0x11,

			// Offset pointer to a GC:d object. Reads 4/8 bytes from 'offset' and treats those as a
			// pointer relative (offset + sizeof(size_t)), ie. the address just after the pointer.
			relativePtr = 0x21,

			// A relative non-gc pointer to somewhere. This is updated as the object moves, but is
			// never scanned.
			relative = 0x30,

			// An absolute pointer to somewhere within this code section. This is updated whenever
			// the object moves, but is never scanned as it does not point to the start of an object.
			inside = 0x40,

			// A relative (4-byte) pointer to one of the 'pointer' variables in the GcCodeRef
			// itself. The value stored inside 'pointer' must be an actual pointer, since it is
			// scanned by the GC. It can not be used to conveniently store large numbers etc.
			relativeHere = 0x51,

			// An architecture specific pointer that modifies the jump instruction as well, so that
			// we can properly support both long and short jump instructions (if wee need to
			// distinguish them) in an efficient manner by using short jumps where possible and
			// falling back to long jumps where necessary.
			jump = 0x61,

			// Update an entry in a separate table containing unwinding information. 'pointer' is a
			// pointer to the location that shall be updated. Architecture specific.
			unwindInfo = 0x70,

			// ARM-specific version of "relativeHere". Writes an offset to the "pointer" member in
			// this struct at the offset indicated in 'offset'. This offset must be aligned to 4
			// bytes. This option assumes that it updates a 19-bit immediate located at bits 5..23.
			relativeHereImm19 = 0x81,

			// ...
		};

		// Offset inside the code where this reference is located. Does not need to be aligned, that
		// depends entirely on the underlying machine we're working with.
		nat offset;

		// Reference type.
		Kind kind;

		// The pointer to be scanned. In the case where 'kind == inside', stores an offset into this
		// object instead.
		void *pointer;
	};


	/**
	 * Describes allocated code.
	 *
	 * NOTE: It is _not_ possible to store pointers to GcCode objects anywhere except for the stack,
	 * as these are really pointers into an object.
	 *
	 * TODO: Expose to Storm? This is only needed when writing the lowest parts of code-generating
	 * backends, which is only really neccessary when porting Storm to a new platform, so this has a
	 * very low priority.
	 */
	struct GcCode {
		// Number of entries. Only changed on allocation.
		const size_t refCount;

		// Reserved for internal use by the GC. Do not alter.
		void *reserved;

		// References in here.
		GcCodeRef refs[1];
	};

}
