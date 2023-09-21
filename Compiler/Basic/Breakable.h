#pragma once
#include "Block.h"

namespace storm {
	namespace bs {
		STORM_PKG(lang.bs);

		/**
		 * A block we can run break- and continue- statements inside, such as loops.
		 */
		class Breakable : public Block {
			STORM_ABSTRACT_CLASS;
		public:
			STORM_CTOR Breakable(SrcPos pos, Scope scope);
			STORM_CTOR Breakable(SrcPos pos, Block *parent);

			// Call to notify this block that we will perform a 'break' at some point.
			virtual void STORM_FN willBreak() ABSTRACT;

			// Call to notify this block that we will perform a 'continue' at some point.
			virtual void STORM_FN willContinue() ABSTRACT;

			// State describing where to jump to on break- and continue statements.
			class To {
				STORM_VALUE;
			public:
				To(code::Label lbl, code::Block block);

				code::Label label;
				code::Block block;
			};

			// Get where to jump on a 'break'. Call during codegen.
			virtual To STORM_FN breakTo() ABSTRACT;

			// Get where to jump on a 'continue'. Call during codegen.
			virtual To STORM_FN continueTo() ABSTRACT;
		};


		/**
		 * Break expression.
		 */
		class Break : public Expr {
			STORM_CLASS;
		public:
			STORM_CTOR Break(SrcPos pos, Block *parent);

			// Result.
			virtual ExprResult STORM_FN result();

			// Generate code.
			virtual void STORM_FN code(CodeGen *state, CodeResult *r);

			// Don't need to isolate this statement.
			virtual Bool STORM_FN isolate();

		protected:
			// To string.
			virtual void STORM_FN toS(StrBuf *to) const;

		private:
			// Block we will break out of.
			Breakable *breakFrom;
		};


		/**
		 * Continue expression.
		 */
		class Continue : public Expr {
			STORM_CLASS;
		public:
			STORM_CTOR Continue(SrcPos pos, Block *parent);

			// Result.
			virtual ExprResult STORM_FN result();

			// Generate code.
			virtual void STORM_FN code(CodeGen *state, CodeResult *r);

			// Don't need to isolate this statement.
			virtual Bool STORM_FN isolate();

		protected:
			// To string.
			virtual void STORM_FN toS(StrBuf *to) const;

		private:
			// Block we will continue in.
			Breakable *continueIn;
		};

	}
}
