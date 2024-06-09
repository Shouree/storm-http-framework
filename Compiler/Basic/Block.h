#pragma once
#include "Expr.h"
#include "Var.h"
#include "Core/SrcPos.h"
#include "Compiler/Scope.h"
#include "Core/Map.h"

namespace storm {
	namespace bs {
		STORM_PKG(lang.bs);

		class BlockLookup;

		/**
		 * A fundamental block. This block only acts as a new scope for variables. It
		 * is abstract to let other types of expressions act as some kind of block
		 * for variables.
		 */
		class Block : public Expr {
			STORM_CLASS;
		public:
			STORM_CTOR Block(SrcPos pos, Scope scope);
			STORM_CTOR Block(SrcPos pos, Block *parent);

			// Lookup node.
			BlockLookup *lookup;

			// Scope.
			Scope scope;

			// Generate code. Override 'blockCode' to generate only block contents.
			virtual void STORM_FN code(CodeGen *state, CodeResult *to);

			// Override to initialize the block yourself.
			virtual void STORM_FN blockCode(CodeGen *state, CodeResult *to, code::Block newBlock);

			// Override to generate contents of the block.
			virtual void STORM_FN blockCode(CodeGen *state, CodeResult *to);

			// Find a variable. Same semantics as 'find'.
			virtual MAYBE(LocalVar *) STORM_FN variable(SimplePart *name);

			// Find a variable in this particular block. Does not recurse as "variable" does.
			virtual MAYBE(LocalVar *) STORM_FN variableHere(SimplePart *name);

			// Add a variable
			virtual void STORM_FN add(LocalVar *v);

			// Lift all variables present inside 'o' into this block. Can only be used one step at a
			// time to not cause strange scoping issues.
			virtual void STORM_FN liftVars(Block *from);

			// Get our parent block, if any.
			MAYBE(Block *) STORM_FN parent();

			// Don't need to isolate blocks, we do that ourselves.
			virtual Bool STORM_FN isolate();

		protected:
			// Initialize variables in a scope, if you're overriding "code" directly.
			void STORM_FN initVariables(CodeGen *child);

		private:
			// Variables in this block.
			typedef Map<Str *, LocalVar *> VarMap;
			Map<Str *, LocalVar *> *variables;

			// Check if 'x' is a child to us.
			bool isParentTo(Block *x);

		};

		/**
		 * A block that contains statements.
		 */
		class ExprBlock : public Block {
			STORM_CLASS;
		public:
			STORM_CTOR ExprBlock(SrcPos pos, Scope scope);
			STORM_CTOR ExprBlock(SrcPos pos, Block *parent);

			// Add an expression.
			void STORM_FN add(Expr *s);
			using Block::add;

			// Add an expression to a specific location.
			void STORM_FN insert(Nat pos, Expr *s);

			// Result.
			virtual ExprResult STORM_FN result();

			// Optimization.
			virtual void STORM_FN code(CodeGen *state, CodeResult *to);

			// Code generation.
			virtual void STORM_FN blockCode(CodeGen *state, CodeResult *to);

			// Auto-casting should work across the boundaries of an expression.
			virtual Int STORM_FN castPenalty(Value to);

			// Get expression at location i.
			Expr *STORM_FN operator [](Nat i) const;

			// Get number of expressions.
			Nat STORM_FN count() const;

		protected:
			// To string.
			virtual void STORM_FN toS(StrBuf *to) const;

			// Expressions here.
			Array<Expr *> *exprs;
		};


		/**
		 * Node in the name lookup tree for our blocks.
		 */
		class BlockLookup : public NameLookup {
			STORM_CLASS;
		public:
			// Create.
			STORM_CTOR BlockLookup(Block *block, NameLookup *prev);

			// Associated block.
			Block *block;

			// Find a variable here.
			virtual MAYBE(Named *) STORM_FN find(SimplePart *part, Scope source);
		};

	}
}
