#pragma once
#include "Reg.h"
#include "OpCode.h"
#include "CondFlag.h"
#include "Var.h"
#include "Block.h"
#include "Label.h"
#include "Reference.h"
#include "Core/SrcPos.h"

namespace code {
	STORM_PKG(core.asm);

	/**
	 * Class describing an operand for an asm instruction. May be either:
	 * - a constant
	 * - a register
	 * - a pointer
	 * - a pointer relative a register
	 * - a label
	 * - a reference to something else in the Arena
	 * - a variable
	 * - a block
	 * - a condition flag
	 *
	 * Not all kinds of operands are valid for all kind of operations. Use the creator functions in
	 * 'Instruction.h' to create instructions, as these enforce the correct types.
	 */

	/**
	 * Type of the operand.
	 */
	enum OpType {
		// No data.
		STORM_NAME(opNone, none),

		// Constant.
		STORM_NAME(opConstant, constant),

		// Dual-constant (one value for 32-bit and one for 64-bit). Appears as 'opConstant', but is stored in 'opOffset'.
		STORM_NAME(opDualConstant, dualConstant),

		// Register
		STORM_NAME(opRegister, register),

		// Relative to a register, ie. [reg + offset]
		STORM_NAME(opRelative, relative),

		// Relative to a label. TODO: Implement for X86!
		STORM_NAME(opRelativeLbl, relativeLabel),

		// Variable (+ offset).
		STORM_NAME(opVariable, variable),

		// Label.
		STORM_NAME(opLabel, label),

		// Block.
		STORM_NAME(opBlock, block),

		// Reference to another object.
		STORM_NAME(opReference, reference),

		// Reference to an object.
		STORM_NAME(opObjReference, objReference),

		// Condition flag.
		STORM_NAME(opCondFlag, condFlag),

		// Source code reference (SrcPos).
		STORM_NAME(opSrcPos, srcPos),
	};

	/**
	 * The operand itself.
	 */
	class Operand {
		STORM_VALUE;
	public:
		// No value.
		STORM_CTOR Operand();

		// Register.
		STORM_CAST_CTOR Operand(Reg reg);

		// CondFlag.
		STORM_CAST_CTOR Operand(CondFlag flag);

		// Variable.
		STORM_CAST_CTOR Operand(Var var);

		// Block or part.
		STORM_CAST_CTOR Operand(Block part);

		// Label.
		STORM_CAST_CTOR Operand(Label label);

		// Reference.
		STORM_CAST_CTOR Operand(Ref ref);
		STORM_CAST_CTOR Operand(Reference *ref);

		// SrcPos.
		STORM_CAST_CTOR Operand(SrcPos pos);

		/**
		 * Operations.
		 */

		// Compare.
		Bool STORM_FN operator ==(const Operand &o) const;
		Bool STORM_FN operator !=(const Operand &o) const;

		// Empty?
		Bool STORM_FN empty() const;
		Bool STORM_FN any() const;

		// Get the type of the operand.
		OpType STORM_FN type() const;

		// Get the size of the operand.
		Size STORM_FN size() const;

		// Is this operand readable?
		Bool STORM_FN readable() const;

		// Is this operand writable?
		Bool STORM_FN writable() const;

		// Throw an exception unless the value is readable/writable.
		void STORM_FN ensureReadable(op::OpCode op) const;
		void STORM_FN ensureWritable(op::OpCode op) const;

		// Does this operand use a register (Note: local variables that will use ptrBase later are
		// not considered to be using a register). This currently includes raw registers and memory
		// accesses relative to registers.
		Bool STORM_FN hasRegister() const;

		/**
		 * Get values. Throws an error if used incorrectly.
		 */

		// Get the constant in the operand. Valid if `type` returns `constant`. If the value stored is
		// a `Size` or an `Offset`, then the value obtained by calling `current` is returned.
		Word STORM_FN constant() const;

		// Get the content register. Valid if `type` returns `register` or `relative`.
		Reg STORM_FN reg() const;

		// Get the offset used. Possible to call any time, but only makes sense for `relative`,
		// `variable`, and `relativeLabel`.
		Offset STORM_FN offset() const;

		// Get the condition flag. Valid if `type` returns `condFlag`.
		CondFlag STORM_FN condFlag() const;

		// Get the block. Valid if `type` returns `block`.
		Block STORM_FN block() const;

		// Get the label. Valid if `type` returns `label`.
		Label STORM_FN label() const;

		// Get the reference. Valid if `type` returns `reference`.
		Ref STORM_FN ref() const;

		// Convenient internal access.
		RefSource *refSource() const;

		// Get the variable accessed. Valid if `type` returns `variable`.
		//
		// Note: The size of the returned variable is equal to the size we wish to read. This
		// may differ from the size of the original variable!
		Var STORM_FN var() const;

		// Get the source position. Valid if `type` returns `srcPos`.
		SrcPos STORM_FN srcPos() const;

		// Get the object from C++.
		RootObject *object() const;

		// Get the referenced object. Valid if `type` returns `objReference`. Returns an `Object`.
		MAYBE(Object *) STORM_FN obj() const;

		// Get the referenced object. Valid if `type` returns `objReference`. Returns a `TObject`.
		MAYBE(TObject *) STORM_FN tObj() const;

		// Replace the register in this operand. If no register, this is a no-op.
		Operand STORM_FN replaceRegister(Reg replace) const;

		/**
		 * Debug.
		 */

		void dbg_dump() const;

		// Output.
		void STORM_FN toS(StrBuf *to) const;

	private:
		// Create constants.
		Operand(Word c, Size size);
		Operand(Size s, Size size);
		Operand(Offset o, Size size);
		Operand(RootObject *obj);

		// [Register,variable,label] + offset.
		Operand(Reg r, Offset offset, Size size);
		Operand(Var v, Offset offset, Size size);
		Operand(Label l, Offset offset, Size size);

		// Our type. Note: may contain other flags as well!
		OpType opType;

		/**
		 * Storage. Optimized for small storage, while not using unions (as they are not supported by the CppTypes).
		 */

		// Store pointers here (ie. references, etc.)
		UNKNOWN(PTR_GC) void *opPtr;

		// Constant data/size data/offset data.
		Word opNum;

		// Offset (if needed).
		Offset opOffset;

		// Size of our data.
		Size opSize;

		// Friends.
		friend Operand intConst(Offset v);
		friend Operand natConst(Size v);
		friend Operand ptrConst(Size v);
		friend Operand ptrConst(Offset v);
		friend Operand xConst(Size s, Word w);
		friend Operand objPtr(Object *ptr);
		friend Operand objPtr(TObject *ptr);
		friend Operand xRel(Size size, Reg reg, Offset offset);
		friend Operand xRel(Size size, Var v, Offset offset);
		friend Operand xRel(Size size, Label l, Offset offset);
		friend wostream &operator <<(wostream &to, const Operand &o);
	};

	// Output.
	wostream &operator <<(wostream &to, const Operand &o);

	// Create constants. Note: Do not try to create a pointer to a Gc:d object using xConst. That
	// will fail miserably, as the pointer will not be updated whenever the Gc moves the object.
	Operand STORM_FN byteConst(Byte v);
	Operand STORM_FN intConst(Int v);
	Operand STORM_FN intConst(Offset v);
	Operand STORM_FN natConst(Nat v);
	Operand STORM_FN natConst(Size v);
	Operand STORM_FN longConst(Long v);
	Operand STORM_FN wordConst(Word v);
	Operand STORM_FN floatConst(Float v);
	Operand STORM_FN doubleConst(Double v);
	Operand STORM_FN ptrConst(Size v);
	Operand STORM_FN ptrConst(Offset v);
	Operand STORM_FN ptrConst(Nat v);
	Operand STORM_FN xConst(Size s, Word v);

	// Store a pointer to a GC:d object as a constant.
	Operand STORM_FN objPtr(Object *ptr);
	Operand STORM_FN objPtr(TObject *ptr);

	// Relative values (dereference pointers).
	Operand STORM_FN byteRel(Reg reg, Offset offset);
	Operand STORM_FN intRel(Reg reg, Offset offset);
	Operand STORM_FN longRel(Reg reg, Offset offset);
	Operand STORM_FN floatRel(Reg reg, Offset offset);
	Operand STORM_FN doubleRel(Reg reg, Offset offset);
	Operand STORM_FN ptrRel(Reg reg, Offset offset);
	Operand STORM_FN xRel(Size size, Reg reg, Offset offset);

	Operand STORM_FN byteRel(Reg reg);
	Operand STORM_FN intRel(Reg reg);
	Operand STORM_FN longRel(Reg reg);
	Operand STORM_FN floatRel(Reg reg);
	Operand STORM_FN doubleRel(Reg reg);
	Operand STORM_FN ptrRel(Reg reg);
	Operand STORM_FN xRel(Size size, Reg reg);

	// Access offsets inside variables.
	Operand STORM_FN byteRel(Var v, Offset offset);
	Operand STORM_FN intRel(Var v, Offset offset);
	Operand STORM_FN longRel(Var v, Offset offset);
	Operand STORM_FN floatRel(Var v, Offset offset);
	Operand STORM_FN doubleRel(Var v, Offset offset);
	Operand STORM_FN ptrRel(Var v, Offset offset);
	Operand STORM_FN xRel(Size size, Var v, Offset offset);

	Operand STORM_FN byteRel(Var v);
	Operand STORM_FN intRel(Var v);
	Operand STORM_FN longRel(Var v);
	Operand STORM_FN floatRel(Var v);
	Operand STORM_FN doubleRel(Var v);
	Operand STORM_FN ptrRel(Var v);
	Operand STORM_FN xRel(Size size, Var v);

	// Access offsets inside labels.
	Operand STORM_FN byteRel(Label l, Offset offset);
	Operand STORM_FN intRel(Label l, Offset offset);
	Operand STORM_FN longRel(Label l, Offset offset);
	Operand STORM_FN floatRel(Label l, Offset offset);
	Operand STORM_FN doubleRel(Label l, Offset offset);
	Operand STORM_FN ptrRel(Label l, Offset offset);
	Operand STORM_FN xRel(Size size, Label l, Offset offset);

	Operand STORM_FN byteRel(Label l);
	Operand STORM_FN intRel(Label l);
	Operand STORM_FN longRel(Label l);
	Operand STORM_FN floatRel(Label l);
	Operand STORM_FN doubleRel(Label l);
	Operand STORM_FN ptrRel(Label l);
	Operand STORM_FN xRel(Size size, Label l);
}
