#pragma once
#include "Block.h"

namespace storm {
	namespace bs {
		STORM_PKG(lang.bs);

		class ReturnInfo;

		/**
		 * Return statement.
		 */
		class Return : public Expr {
			STORM_CLASS;
		public:
			// Return nothing.
			STORM_CTOR Return(SrcPos pos, Block *block);

			// Return a value.
			STORM_CTOR Return(SrcPos pos, Block *block, Expr *expr);

			// Value to return.
			MAYBE(Expr *) expr;

			// Result (never returns).
			virtual ExprResult STORM_FN result();

			// Generate code.
			virtual void STORM_FN code(CodeGen *state, CodeResult *r);

		protected:
			// Output.
			virtual void STORM_FN toS(StrBuf *to) const;

		private:
			// Function body we are returning from.
			ReturnInfo *retInfo;

			// Check type.
			void checkType();

			// Find the surrounding ReturnTo.
			ReturnInfo *findReturn(SrcPos pos, Block *block);

			// Code generation.
			void voidCode(CodeGen *state);
			void valueCode(CodeGen *state);
		};


		/**
		 * Information about where to return to.
		 */
		class ReturnInfo : public ObjectOn<Compiler> {
			STORM_CLASS;
		public:
			// Create, initialize with a type.
			STORM_CTOR ReturnInfo(Value type);

			// Type that should be returned.
			Value type;

			// If present, we are generating code in an inline function. That means that the result
			// should be stored inside this CodeResult, and execution should jump to the label
			// indicated below. If null, we are being used as a top-level function.
			MAYBE(CodeResult *) inlineResult;

			// Label to jump to when we return from an inline function.
			code::Label inlineLabel;

			// Block to jump out to.
			code::Block inlineBlock;

			// Check the result. Called by "Return"'s constructor. Modifies "expr" to return an
			// appropriate type. Throws on error. If 'expr' is null, then the implementation assumes
			// an empty return statement (i.e. returning void).
			virtual MAYBE(Expr *) STORM_FN checkResult(SrcPos pos, Scope scope, MAYBE(Expr *) expr);
		};


		/**
		 * A block that is possible to return from, as an expression.
		 */
		class ReturnPoint : public Block {
			STORM_CLASS;
		public:
			STORM_CTOR ReturnPoint(SrcPos pos, Scope scope, Value type);
			STORM_CTOR ReturnPoint(SrcPos pos, Block *parent, Value type);
			STORM_CTOR ReturnPoint(SrcPos pos, Scope scope, ReturnInfo *info);
			STORM_CTOR ReturnPoint(SrcPos pos, Block *parent, ReturnInfo *info);

			// Compute the result.
			virtual ExprResult STORM_FN result();

			// Generate code.
			virtual void STORM_FN blockCode(CodeGen *state, CodeResult *to, code::Block newBlock);

			// Return information.
			ReturnInfo *info;

			// Set contained expression.
			void STORM_ASSIGN body(Expr *e);

			// Print.
			virtual void STORM_FN toS(StrBuf *to) const;

		private:
			// Body expression.
			MAYBE(Expr *) bodyExpr;
		};

	}
}
