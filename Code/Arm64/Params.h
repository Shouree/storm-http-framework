#pragma once
#include "../TypeDesc.h"
#include "../Reg.h"
#include "Core/Object.h"
#include "Core/Array.h"
#include "Utils/Bitwise.h"

namespace code {
	namespace arm64 {
		STORM_PKG(core.asm.arm64);

		/**
		 * Implements the logic for laying out parameters on the stack and into registers.
		 *
		 * On ARM64 parameters are passed according to the following rules:
		 * - The first eight integer or pointer parameters are passed in registers x0..x7
		 * - The first eight floating-point arguments are passed in registers d0..d7
		 * - Any parameters that do not fit in registers are passed on the stack.
		 * - Composite types that are not "trivial" are passed on the stack.
		 * - Composite types are passed on the stack if they are larger than 16 bytes.
		 * - Composite types of size up to 16 bytes might be split into two registers.
		 *   If this is done, the register number is rounded up to the next even number.
		 * - Note: Arguments copied to the stack will have to be aligned to 8 bytes, even
		 *   if their natural alignment is smaller.
		 * - Floating-point values are passed in fp registers. If int- and float parameters
		 *   overlap, they are passed in int registers. This is also true when a struct
		 *   contains two different words, and they could be passed in different types
		 *   of registers.
		 *
		 * In summary, some important differences from the X86-64 calling convention are:
		 * - Each parameter gets allocated to *one* set of registers. It is never split between
		 *   integer registers and fp registers.
		 * - Return values are handled as "regular" parameters.
		 * - No special cases with regards to the "this" parameter.
		 * - Register numbers are "aligned" when large structs are passed.
		 * - FP registers are *only* used for uniform float parameters. I.e., not if a struct
		 *   contains e.g. 2 floats and 1 double.
		 */

		/**
		 * Describes what a single register is supposed to contain.
		 */
		class Param {
			STORM_VALUE;
		public:
			// Create empty parameter.
			STORM_CTOR Param();
			STORM_CTOR Param(Nat id, Primitive p);
			STORM_CTOR Param(Nat id, Nat size, Nat offset, Bool stack);

			// Reset.
			void clear();

			// Set to values.
			void set(Nat id, Nat size, Nat offset, Bool stack);

			// Get the different fields stored in here:
			inline Nat size() const {
				return data & 0xF;
			}
			inline Bool stack() const {
				return (data & 0x10) != 0;
			}
			inline Nat id() const {
				return (data >> 5) & 0xFF;
			}
			inline Nat offset() const {
				return data >> 13;
			}

			// Compare for equality.
			inline Bool STORM_FN operator ==(Param o) const {
				return data == o.data;
			}
			inline Bool STORM_FN operator !=(Param o) const {
				return data != o.data;
			}

			// ID usable for a hidden return parameter.
			enum { returnId = 0xFF };

		private:
			// Stored as follows:
			// Bits 0-3: Size of the register (in bytes).
			// Bit  4: Pointer to stack parameter?
			// Bits 5-12: Parameter number. 0xFF = unused.
			// Bits 13-31: Offset into parameter (in bytes).
			Nat data;
		};

		// Output.
		wostream &operator <<(wostream &to, Param p);
		StrBuf &STORM_FN operator <<(StrBuf &to, Param p);


		/**
		 * Describes how parameters are laid out during a function call. This is specific to ARM64,
		 * so any sizes and offsets stored here are for that specific platform.
		 */
		class Params : public storm::Object {
			STORM_CLASS;
		public:
			// Create an empty parameter layout.
			STORM_CTOR Params();

			// Add a single parameter to here.
			void STORM_FN add(Nat id, TypeDesc *type);

			// Add a primitive.
			void STORM_FN add(Nat id, Primitive p);

			// To string.
			virtual void STORM_FN toS(StrBuf *to) const;

			/**
			 * Access to the stack layout.
			 */

			// Get the total number of elements on the stack.
			Nat STORM_FN stackCount() const {
				return stackPar->count();
			}

			// Get stack element number 'n'.
			Nat STORM_FN stackParam(Nat n) const {
				return stackPar->at(n);
			}

			// Get stack element's offset relative to SP.
			Nat STORM_FN stackOffset(Nat n) const {
				return stackOff->at(n);
			}

			// Get total size of stack.
			Nat STORM_FN stackTotalSize() const {
				// Must be 16-byte aligned.
				return roundUp(stackSize, Nat(16));
			}

			/**
			 * Access to the registers.
			 */

			// Total number of registers to examine.
			Nat STORM_FN registerCount() const;

			// Get a parameter. Might be empty.
			Param STORM_FN registerAt(Nat n) const;

			// Get the register containing the parameter.
			Reg STORM_FN registerSrc(Nat n) const;

		private:
			// Available registers:

			// Regular integer registers.
			GcArray<Param> *integer;
			// Real registers, for floating point parameters.
			GcArray<Param> *real;
			// Stores all parameters that are to be passed on the stack.
			Array<Nat> *stackPar;
			// Stores offsets of all stack parameters, relative to SP on function call entry.
			Array<Nat> *stackOff;
			// Total stack size.
			Nat stackSize;

			// Add complex and simple structs.
			void addDesc(Nat id, ComplexDesc *type);
			void addDesc(Nat id, SimpleDesc *type);

			// Try to add a parameter to 'to', otherwise add it to the stack.
			void addParam(GcArray<Param> *to, Param p);

			// Push parameter onto stack. Assumes "desc" will be copied onto the stack.
			void addStack(Nat id, TypeDesc *desc);
			void addStack(Nat id, Size size);
		};

		// Create a 'params' object from a list of TypeDesc objects.
		Params *STORM_FN layoutParams(Array<TypeDesc *> *types);


		/**
		 * Describes how the return value is stored.
		 *
		 * On ARM64, the rule is that if the function "fn(X)" would pass "X" in register, the same
		 * registers are used for returning a value of type "X". Otherwise, an address to memory is
		 * passed in x8.
		 */
		class Result : public storm::Object {
			STORM_CLASS;
		public:
			// Create.
			STORM_CTOR Result(TypeDesc *type);

			// What types of registers to use for the result. They are always of the same type on Arm64.
			primitive::PrimitiveKind regType;

			// Using two registers?
			Bool twoReg;

			// Return in memory?
			Bool memory;

			// To string.
			virtual void STORM_FN toS(StrBuf *to) const;

		private:
			// Helper functions.
			void add(PrimitiveDesc *desc);
			void add(ComplexDesc *desc);
			void add(SimpleDesc *desc);
		};

		// Create a 'result' object describing how the return value shall be represented.
		Result *STORM_FN result(TypeDesc *type);

	}
}
