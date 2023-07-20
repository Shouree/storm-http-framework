#pragma once
#include "Reg.h"
#include "TypeDesc.h"
#include "Core/Object.h"
#include "Utils/Bitwise.h"

namespace code {
	STORM_PKG(core.asm);

	/**
	 * Generic representation of how a set of parameters are allocated to a set of registers for
	 * function calls.
	 *
	 * This file contains the generic parts of the representation. That is:
	 * - a class that represents what part of a parameter is to end up in a particular location and
	 * - a class that stores a set of these representations, both for available registers and for
	 *   the stack.
	 *
	 * The actual logic is implemented by each of the backends.
	 */

	/**
	 * Describes what parameter is to be in some particular location.
	 *
	 * Note: Only saves either 32-bit or 64-bit size of the parameter as it is typically not
	 * relevant to make platform-agnostic stack layouts.
	 *
	 * Note: the use64 parameter determines whether to use the 32-bit or 64-bit size of the type.
	 */
	class Param {
		STORM_VALUE;
	public:
		// Empty parameter.
		STORM_CTOR Param();

		// Create a representation of parameter number "id" from a whole "primitive".
		STORM_CTOR Param(Nat id, Primitive p, Bool use64);

		// Create from parameter id, size, offset and wheter the parameter is passed in memory or
		// not (i.e. stored as a pointer).
		STORM_CTOR Param(Nat id, Size size, Bool use64, Nat offset, Bool inMemory);

		// Any contents?
		Bool STORM_FN any() { return !empty(); }
		Bool STORM_FN empty();

		// Make empty.
		void STORM_FN clear();

		// Set to particular values.
		void STORM_FN set(Nat id, Size size, Bool use64, Nat offset, Bool inMemory);

		// ID usable for the return value.
		static Nat STORM_FN returnId() { return 0xFF; }

		/**
		 * Get the different fields:
		 */

		// Get the size of the parameter. This is the size of the data stored in the
		// parameter-passing code. If 'inMemory' is true this function will thus always return sPtr.
		inline Size STORM_FN size() const {
			if (inMemory())
				return Size::sPtr;
			else
				return realSize();
		}

		// Get the "real" size of the parameter. This refers to the size of the actual parameter,
		// possibly after indirection.
		inline Size STORM_FN realSize() const {
			Nat sz = dsize >> 4;
			Nat align = dsize & 0xF;
			return Size(sz, align, sz, align);
		}

		// Passed in memory through a pointer indirection?
		inline Bool STORM_FN inMemory() const {
			return (data & 0x1) != 0;
		}

		// Parameter number.
		inline Nat STORM_FN id() const {
			return (data >> 1) & 0xFF;
		}

		// Offset into the parameter (i.e., what part of the parameter are we referring to?)
		inline Nat STORM_FN offset() const {
			return data >> 9;
		}

		// Compare for equality.
		inline Bool STORM_FN operator ==(Param o) const {
			return data == o.data;
		}
		inline Bool STORM_FN operator !=(Param o) const {
			return data != o.data;
		}

	private:
		// Stored as follows:

		// Packed size of the data. We only store either 32- or 64-bit data.
		// Bits 0-3: offset
		// Bits 4-31: size
		Nat dsize;

		// Other fields:
		// Bit  0: 'inMemory'?
		// Bits 1-8: Parameter number.
		// Bits 9-31: Offset into the parameter (bytes)
		Nat data;
	};

	// Output.
	wostream &operator <<(wostream &to, Param p);
	StrBuf &STORM_FN operator <<(StrBuf &to, Param p);


	/**
	 * Description of how to store the results from a function call.
	 *
	 * Similarly to Params, this contains a set of registers to which the return value is
	 * allocated. The result might also be passed in memory. In that case, the address of the value
	 * should be passed in the indicated register.
	 */
	class Result {
		STORM_VALUE;
	public:
		// Create empty result.
		STORM_CTOR Result();

		// Other way of creating empty result.
		static Result STORM_FN empty() { return Result(); }

		// Create, indicate that result is to be returned in memory.
		static Result STORM_FN inMemory(Reg reg);

		// Create, allocate a specified number of registers.
		static Result STORM_FN inRegisters(EnginePtr e, Nat count);

		// Create, store in a single register. Equivalent to creating and adding register in separate steps.
		static Result STORM_FN inRegister(EnginePtr e, Reg reg);

		// Set register data.
		void putRegister(Reg reg, Nat offset);

		// If the value is to be passed in memory, this function returns a register that should
		// store the address where the result is to be stored. Otherwise returns noReg.
		Reg STORM_FN memoryRegister() const { return memReg; }

		// Get number of registers.
		Nat STORM_FN registerCount() const {
			return regs ? Nat(regs->filled) : 0;
		}

		// Get register to store part of data inside. The register size indicates the size of the data.
		Reg STORM_FN registerAt(Nat id) const {
			return regs->v[id].reg;
		}

		// Get the offset of the input to store inside 'registerAt'.
		Offset STORM_FN registerOffset(Nat id) const {
			return Offset(regs->v[id].offset);
		}

	private:
		// Register to store return address in.
		Reg memReg;

		// Data for register assignments.
		struct Data {
			Reg reg;
			Nat offset;
		};
		GcArray<Data> *regs;

		// GC types:
		static const GcType dataType;
	};


	// Output.
	wostream &operator <<(wostream &to, Result p);
	StrBuf &STORM_FN operator <<(StrBuf &to, Result p);


	/**
	 * Describes the layout of parameters during a function call for some platform.
	 *
	 * This class is abstract as it lacks the layout logic. Use the Params class from a respective
	 * backend to get this behavior for the appropriate architecture.
	 *
	 * Note: parameters "passed on the stack" are the parameters that did not fit in registers and
	 * were pushed to the stack. This is different from parameters that are passed in memory. The
	 * latter parameters can typically be located anywhere in memory, and their address is passed as
	 * a pointer. It is therefore possible for the stack to contain pointers to parameters that were
	 * spilled to memory.
	 */
	class Params : public storm::Object {
		STORM_ABSTRACT_CLASS;
	public:
		// Create an empty layout. Pre-allocate registers and specify stack alignment.
		STORM_CTOR Params(Nat intCount, Nat realCount, Nat stackParamAlign, Nat stackAlign);

		// Set the result type. Must be done before adding parameters.
		void STORM_FN result(TypeDesc *type);
		void STORM_FN result(Primitive p);

		// Get the result.
		Result STORM_FN result() {
			return resultData;
		}

		// Add a single parameter to the layout.
		void STORM_FN add(Nat id, TypeDesc *type);

		// Add a primitive to the layout.
		void STORM_FN add(Nat id, Primitive p);

		// To string.
		virtual void STORM_FN toS(StrBuf *to) const;

		/**
		 * Access to the layout of the stack.
		 */

		// Get the total number of elements on the stack.
		Nat STORM_FN stackCount() const {
			return stackPar ? Nat(stackPar->filled) : 0;
		}

		// Get stack element number 'n'.
		Param STORM_FN stackParam(Nat n) const {
			return stackPar->v[n].param;
		}

		// Get stack element's offset relative to SP.
		Nat STORM_FN stackOffset(Nat n) const {
			return stackPar->v[n].offset;
		}

		// Get total size of the stack.
		Nat STORM_FN stackTotalSize() const {
			return roundUp(stackSize, stackAlign);
		}

		// Get unaligned size of the stack.
		Nat STORM_FN stackTotalSizeUnaligned() const {
			return stackSize;
		}

		/**
		 * Access to registers.
		 */

		// Total number of registers to examine.
		Nat STORM_FN registerCount() const {
			return Nat(integer->count + real->count);
		}

		// Get the contents of a register. Might be empty.
		Param STORM_FN registerParam(Nat n) const {
			if (n < integer->count)
				return integer->v[n];
			n -= Nat(integer->count);
			if (n < real->count)
				return real->v[n];
			return Param();
		}

		// Get register to use for a particular parameter.
		virtual Reg STORM_FN registerSrc(Nat n) const ABSTRACT;

		/**
		 * Iterate through all parameters.
		 */

		// Total number of elements.
		Nat STORM_FN totalCount() const {
			return registerCount() + stackCount();
		}

		// Get element at position.
		Param STORM_FN totalParam(Nat id) const;

	protected:
		// Dispatched to subclasses from "result":
		virtual void STORM_FN resultPrimitive(Primitive p) ABSTRACT;
		virtual void STORM_FN resultComplex(ComplexDesc *c) ABSTRACT;
		virtual void STORM_FN resultSimple(SimpleDesc *s) ABSTRACT;

		// Dispatched to subclasses from "add":
		virtual void STORM_FN addPrimitive(Nat id, Primitive p) ABSTRACT;
		virtual void STORM_FN addComplex(Nat id, ComplexDesc *c) ABSTRACT;
		virtual void STORM_FN addSimple(Nat id, SimpleDesc *s) ABSTRACT;

		// Used to add parameters as appropriate. AddInt and addReal falls back to adding onto the
		// stack if there is no space available.
		void STORM_FN addInt(Param param);
		void STORM_FN addReal(Param param);
		void STORM_FN addStack(Param param);

		// Check available space:
		Bool STORM_FN hasInt(Nat count);
		Bool STORM_FN hasReal(Nat count);

		// Representation of the result. Returned from the 'result' call.
		Result resultData;

	private:
		// Available integer registers (pre-allocated):
		GcArray<Param> *integer;
		// Available fp registers (pre-allocated):
		GcArray<Param> *real;

		// Parameters on the stack:
		struct StackParam {
			Param param;
			Nat offset;
		};

		// All parameters passed on the stack (dynamic array, or null).
		GcArray<StackParam> *stackPar;

		// Total stack size.
		Nat stackSize;

		// Alignment of the stack.
		Nat stackAlign;

		// Alignment of each parameter on the stack.
		Nat stackParamAlign;

		// GC types:
		static const GcType paramType;
		static const GcType stackParamType;
	};

}
