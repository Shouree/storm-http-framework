#pragma once
#include "Core/Object.h"
#include "Core/Array.h"
#include "TypeDesc.h"
#include "Instr.h"
#include "Label.h"
#include "Var.h"
#include "Block.h"

namespace code {
	STORM_PKG(core.asm);

	class Arena;

	/**
	 * Control when and how variables are freed.
	 */
	enum FreeOpt {
		STORM_NAME(freeOnNone, none) = 0x0,
		STORM_NAME(freeOnException, exception) = 0x1,
		STORM_NAME(freeOnBlockExit, blockExit) = 0x2,
		STORM_NAME(freeOnBoth, both) = 0x3,

		// Pass a pointer to the free-function?
		STORM_NAME(freePtr, ptr) = 0x10,

		// Used by the backends: What is stored in the variable is actually a pointer to the value.
		STORM_NAME(freeIndirection, indirection) = 0x20,

		// Default options.
		STORM_NAME(freeDef, default) = freeOnBoth,

		// This variable needs to be activated before it will be freed.
		STORM_NAME(freeInactive, inactive) = 0x40,
	};

	BITMASK_OPERATORS(FreeOpt);

	wostream &operator <<(wostream &to, FreeOpt o);
	StrBuf &operator <<(StrBuf &to, FreeOpt o);


	/**
	 * Represents a code listing along with information about blocks and variables. Can be linked
	 * into machine code.
	 *
	 * A listing contains a set of labels. Each instruction may have zero or more labels attached to
	 * them. The labels can then be referenced inside that listing, which creates a reference to the
	 * start of the attached instruction.
	 *
	 * A listing also contains a set of blocks. By default, there is a root block. Each block may
	 * then contain zero or more nested blocks, and zero or more variables. The root block is active
	 * from the prolog() instruction to the epilog() instruction. Whenever a block is active, all
	 * variables inside it and its parent blocks are accessible.
	 *
	 * Each variable may also be associated with an "activation point" by setting "freeInactive"
	 * flag on the variable. This means that the variable will be accessible as usual, but that it
	 * will not be destroyed until it has been explicitly activated. This is used to properly handle
	 * values that need to be present in memory when initializing them, but destruction shall not be
	 * done until the constructor is finished. Variables are activated using the activate()
	 * pseudo-instruction. As with blocks, these are interpreted in a lexical scope. I.e. a variable
	 * is considered inactive in all locations before the activate() instruction and active in all
	 * locations after activate(), regardless of jumps and which blocks are entered.
	 *
	 * Note: any labels which have not had an instruction added after them are considered
	 * unused. Ie. it is not possible to reference the end of the listing, as that feature is seldom
	 * used. If you need labels to the end, consider adding a dummy dat(0) at the end.
	 */
	class Listing : public Object {
		STORM_CLASS;
	public:
		// Create an empty listing. Optionally associating it with an arena. Doing that makes the
		// listing show backend-specific things in a proper way.
		STORM_CTOR Listing();
		STORM_CTOR Listing(Bool member, TypeDesc *result);
		STORM_CTOR Listing(const Arena *arena);
		STORM_CTOR Listing(const Arena *arena, Bool member, TypeDesc *result);

		Listing(const Listing &o);

		// Deep copy.
		virtual void STORM_FN deepCopy(CloneEnv *env);

		/**
		 * One entry in the listing:
		 */
		class Entry {
			STORM_VALUE;
		public:
			STORM_CTOR Entry();
			STORM_CTOR Entry(Instr *instr);
			STORM_CTOR Entry(Instr *instr, MAYBE(Array<Label> *) labels);

			// Deep copy.
			void STORM_FN deepCopy(CloneEnv *env);

			// The instruction at this point.
			Instr *instr;

			// Labels before this instruction (if any). Only created if any labels are added.
			MAYBE(Array<Label> *) labels;

			// Add a label here.
			void add(Engine &e, Label label);
		};

		// Add instructions and labels.
		Listing &STORM_FN operator <<(Instr *op);
		Listing &STORM_FN operator <<(Label l);
		Listing &STORM_FN operator <<(MAYBE(Array<Label> *) l);

		// Access instructions.
		inline Nat STORM_FN count() const { return code->count(); }
		inline Bool STORM_FN empty() const { return count() == 0; }
		inline Bool STORM_FN any() const { return count() > 0; }
		inline Instr *STORM_FN operator [](Nat id) const { return at(id); }
		inline Instr *at(Nat id) const { return code->at(id).instr; }

		// Give 'id = count()' to access any labels after the last instruction.
		MAYBE(Array<Label> *) STORM_FN labels(Nat id) const;

		// Create a shell, ie. a Listing containing only the scope information from this listing,
		// thus making variables, blocks from this listing are valid in the shell as well.
		Listing *STORM_FN createShell() const;
		Listing *STORM_FN createShell(MAYBE(const Arena *) arena) const;

		/**
		 * Labels.
		 */

		// Label reserved for metadata.
		Label STORM_FN meta();

		// Create a new label.
		Label STORM_FN label();

		// Get the number of labels allocated (i.e., id:s 0 to labelCount - 1 are used).
		Nat STORM_FN labelCount() const;

		/**
		 * Additional source-level information about variables. Front-ends may tag variables in this
		 * way to provide a better debugging experience, but doing so is optional.
		 */
		class VarInfo : public Object {
			STORM_CLASS;
		public:
			// Name of the variable.
			Str *name;

			// Type of the variable.
			Type *type;

			// Reference?
			Bool ref;

			// Source location.
			SrcPos pos;

			// Create.
			STORM_CTOR VarInfo(Str *name, Type *type, Bool ref, SrcPos pos);

			// Deep copy.
			void STORM_FN deepCopy(CloneEnv *env);
		};

		/**
		 * Scope management.
		 */

		// Get the root block.
		Block STORM_FN root() const;

		// Create a block inside 'parent'. Note that it does not matter if 'parent' is the block or
		// any part inside that block.
		Block STORM_FN createBlock(Block parent);

		// Move a parameter to a specific location.
		void STORM_FN moveParam(Var param, Nat to);

		// Move a variable to the beginning of a block. This impacts memory layout and destruction order.
		void STORM_FN moveFirst(Var var);

		// Get the variable stored just before 'v' in this stack frame. Within a single block, it
		// just returns the variable added before 'v'. If 'v' is the first variable in that block,
		// the last variable of the previous block is returned. This will give all variables visible
		// at the same time as the start variable of the iteration. Parameters are returned lastly.
		Var STORM_FN prev(Var v) const;

		// Get the parent part to a variable or a block.
		Block STORM_FN parent(Block b) const;
		Block STORM_FN parent(Var b) const;

		// See if the variable 'v' is accessible in the part 'p'. This is almost equivalent to
		// checking if any parent blocks of 'p' contains the variable.
		Bool STORM_FN accessible(Var v, Block p) const;

		// See if the part 'q' is an indirect parent to 'parent'.
		Bool STORM_FN isParent(Block parent, Block q) const;

		// Is this a parameter?
		Bool STORM_FN isParam(Var v) const;

		// Get parameter info about a variable.
		MAYBE(TypeDesc *) STORM_FN paramDesc(Var v) const;

		// Get debug info about a variable.
		MAYBE(VarInfo *) STORM_FN varInfo(Var v) const;

		// Set debug info.
		void STORM_FN varInfo(Var v, MAYBE(VarInfo *) info);

		// Get all blocks.
		Array<Block> *STORM_FN allBlocks() const;

		// Get all variables. Always in order, so allVars()[i].key() == i
		Array<Var> *STORM_FN allVars() const;

		// Get all variables in a block.
		Array<Var> *STORM_FN allVars(Block b) const;

		// Get all parameters.
		Array<Var> *STORM_FN allParams() const;

		// Get the destructor for a variable or a parameter.
		Operand STORM_FN freeFn(Var v) const;

		// Get when to free a variable or a parameter.
		FreeOpt STORM_FN freeOpt(Var v) const;
		void STORM_FN freeOpt(Var v, FreeOpt opt);

		/**
		 * Information of how to catch exceptions.
		 *
		 * All catch clauses have the following form:
		 * If an exception of type <type> is thrown, goto <resume> which is assumed to reside in the
		 * block level immediately outside the current block.
		 */
		class CatchInfo {
			STORM_VALUE;
		public:
			// Create. 'type' is the type to catch, 'resume' is where to resume execution.
			STORM_CTOR CatchInfo(Type *type, Label resume);

			// Type.
			Type *type;

			// Resume. Assumed to refer to a location in a block immediately outside the block used
			// as a catch handler.
			Label resume;
		};

		// Add a catch handler to a block. Multiple handlers may be added to a single block. If so,
		// they are evaluated in the order they were added.
		void STORM_FN addCatch(Block block, CatchInfo add);
		void STORM_FN addCatch(Block block, Type *type, Label resume);

		// Get all catch clauses for a block.
		MAYBE(Array<CatchInfo> *) STORM_FN catchInfo(Block block) const;

		// Does this block need cleanup during exception handling?
		inline Bool STORM_FN exceptionCleanup() const { return ehClean; }

		// Does this block catch exceptions?
		inline Bool STORM_FN exceptionCaught() const { return ehCatch; }

		// Does this block deal with exceptions at all? I.e. is either 'exceptionCleanup' or 'exceptionCatched' true?
		inline Bool STORM_FN exceptionAware() const { return ehCatch | ehClean; }

		/**
		 * Create variables.
		 */

		inline Var STORM_FN createVar(Block in, Size size) { return createVar(in, size, Operand(), freeDef); }
		inline Var STORM_FN createVar(Block in, Size size, Operand free) { return createVar(in, size, free, freeDef); }
		Var STORM_FN createVar(Block in, Size size, Operand free, FreeOpt when);
		Var STORM_FN createVar(Block in, TypeDesc *type);
		Var STORM_FN createVar(Block in, TypeDesc *type, FreeOpt when);

		Var STORM_FN createParam(TypeDesc *type);
		Var STORM_FN createParam(TypeDesc *type, FreeOpt when);
		Var STORM_FN createParam(TypeDesc *type, Operand free);
		Var STORM_FN createParam(TypeDesc *type, Operand free, FreeOpt when);

		/**
		 * Convenience functions.
		 */

		inline Var STORM_FN createByteVar(Block in) {
			return createVar(in, Size::sByte);
		}
		inline Var STORM_FN createIntVar(Block in) {
			return createVar(in, Size::sInt);
		}
		inline Var STORM_FN createLongVar(Block in) {
			return createVar(in, Size::sLong);
		}
		inline Var STORM_FN createFloatVar(Block in) {
			return createVar(in, Size::sFloat);
		}

		inline Var STORM_FN createByteParam() {
			return createParam(new (this) PrimitiveDesc(bytePrimitive()));
		}
		inline Var STORM_FN createIntParam() {
			return createParam(new (this) PrimitiveDesc(intPrimitive()));
		}
		inline Var STORM_FN createPtrParam() {
			return createParam(new (this) PrimitiveDesc(ptrPrimitive()));
		}
		inline Var STORM_FN createLongParam() {
			return createParam(new (this) PrimitiveDesc(longPrimitive()));
		}
		inline Var STORM_FN createFloatParam() {
			return createParam(new (this) PrimitiveDesc(floatPrimitive()));
		}

		/**
		 * Result from this listing.
		 */

		// Resulting type from this listing.
		TypeDesc *result;

		// Is this function a member of a class?
		Bool member;


		/**
		 * Misc.
		 */

		// Output.
		virtual void STORM_FN toS(StrBuf *to) const;

		// Arena.
		MAYBE(const Arena *const) arena;

	private:
		// Variable information.
		class IVar {
			STORM_VALUE;
		public:
			// Block containing this variable.
			Nat parent;

			// Size of this variable.
			Size size;

			// Parameter? If so: description of the parameter.
			MAYBE(TypeDesc *) param;

			// Optional variable information.
			MAYBE(VarInfo *) info;

			// Function to call on free.
			Operand freeFn;

			// When to free?
			FreeOpt freeOpt;

			// Create.
			IVar(Nat parent, Size size, TypeDesc *param, Operand freeFn, FreeOpt opt);

			// Deep copy.
			void STORM_FN deepCopy(CloneEnv *env);
		};

		// Block information.
		class IBlock {
			STORM_VALUE;
		public:
			// Parent block.
			Nat parent;

			// All variables declared here.
			Array<Nat> *vars;

			// Catch handlers, if any.
			MAYBE(Array<CatchInfo> *) catchInfo;

			// Create.
			IBlock(Engine &e);
			IBlock(Engine &e, Nat parent);

			// Deep copy.
			void STORM_FN deepCopy(CloneEnv *env);
		};

		// Instructions and labels in here.
		Array<Entry> *code;

		// Labels added at the end (if any).
		Array<Label> *nextLabels;

		// Next free label id.
		Nat nextLabel;

		// Store variables and blocks.
		Array<Nat> *params;
		Array<IVar> *vars;
		Array<IBlock> *blocks;

		// Do we need exception cleanup?
		Bool ehClean;

		// Do we catch exceptions?
		Bool ehCatch;

		// Initialize everything.
		void init(Bool member, TypeDesc *result);

		// Find an ID in a Nat array. Returns size if none exists.
		static Nat findId(Array<Nat> *in, Nat val);

		// Create a variable from its index.
		Var createVar(Nat index) const;

		/**
		 * Output helpers.
		 */

		void putBlock(StrBuf &to, Nat block) const;
		void putVar(StrBuf &to, Nat var) const;
	};

	StrBuf &STORM_FN operator <<(StrBuf &to, Listing::Entry e);

}

#include "InstrFwd.h"
