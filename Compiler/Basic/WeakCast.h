#pragma once
#include "Block.h"
#include "Condition.h"
#include "Compiler/Lib/Maybe.h"

namespace storm {
	namespace bs {
		STORM_PKG(lang.bs);

		/**
		 * A weak cast. This is a condition that implements a type-cast that may fail. This
		 * implementation means that the failure has to be handled appropriately.
		 *
		 * This is an abstract class that implements some of the common functionality for all weak
		 * casts in the system.
		 */
		class WeakCast : public Condition {
			STORM_ABSTRACT_CLASS;
		public:
			// Create.
			STORM_CTOR WeakCast();

			/**
			 * Interface from Condition.
			 *
			 * Note: 'code' and 'trueCode' are expected to be implemented by subclasses.
			 */

			// Get a suitable position for the cast.
			virtual SrcPos STORM_FN pos() ABSTRACT;

			// Get the result variable created by this cast.
			virtual MAYBE(LocalVar *) STORM_FN result();

			// Make it more convenient to implement the 'trueCode' function.
			virtual void STORM_FN trueCode(CodeGen *state);

			/**
			 * New interface implemented by subclasses.
			 */

			// Get the name of the variable we want to save the result of the weak cast to by
			// default. May return null if there is no obvious location to store the result in.
			virtual MAYBE(Str *) STORM_FN overwrite();

			// Get the type of the variable we wish to create.
			virtual Value STORM_FN resultType() ABSTRACT;

			// Called to initialize the created variable in the 'true' branch. Only called if a
			// variable is to be initialized.
			virtual void STORM_FN initCode(CodeGen *state, LocalVar *var) ABSTRACT;

			/**
			 * New functionality provided here.
			 */

			// Set the name of the variable to store the result into.
			void STORM_FN name(syntax::SStr *name);

		protected:
			// Helper for the toS implementation. Outputs the specified variable if applicable.
			void STORM_FN output(StrBuf *to) const;

			// Default implementation of 'overwrite'. Extracts the name from an expression if possible.
			MAYBE(Str *) STORM_FN defaultOverwrite(Expr *expr);

		private:
			// Name of an explicitly specified variable, if any.
			MAYBE(syntax::SStr *) varName;

			// Created variable, if any.
			MAYBE(LocalVar *) created;
		};


		/**
		 * Weak cast using 'as'.
		 */
		class WeakAsCast : public WeakCast {
			STORM_ABSTRACT_CLASS;
		public:
			// Create.
			STORM_CTOR WeakAsCast(Block *block, Expr *expr, SrcName *type);
			STORM_CTOR WeakAsCast(Block *block, Expr *expr, Value to);

			// Get a suitable position for the cast.
			virtual SrcPos STORM_FN pos();

			// Get variable we can write to.
			virtual MAYBE(Str *) STORM_FN overwrite();

			// Get the result type.
			virtual Value STORM_FN resultType();

		protected:
			// Output.
			virtual void STORM_FN toS(StrBuf *to) const;

			// Expression to cast.
			Expr *expr;

			// Where was 'expr' stored in 'code', so that we may access it in 'trueCode'?
			code::Var exprVar;

			// Does 'exprVar' contain a reference?
			Bool exprVarRef;

			// Type to cast to.
			Value to;
		};


		/**
		 * Weak downcast using 'as'.
		 */
		class WeakDowncast : public WeakAsCast {
			STORM_CLASS;
		public:
			// Create.
			STORM_CTOR WeakDowncast(Block *block, Expr *expr, SrcName *type);
			STORM_CTOR WeakDowncast(Block *block, Expr *expr, Value to);

			// Generate code.
			virtual void STORM_FN code(CodeGen *state, CodeResult *boolResult);

			// Generate code to initialize the variable.
			virtual void STORM_FN initCode(CodeGen *state, LocalVar *var);
		};


		/**
		 * Weak cast extracting values from the Variant type.
		 */
		class WeakVariantCast : public WeakAsCast {
			STORM_CLASS;
		public:
			// Create.
			STORM_CTOR WeakVariantCast(Block *Block, Expr *expr, SrcName *type);
			STORM_CTOR WeakVariantCast(Block *block, Expr *expr, Value to);

			// Generate code.
			virtual void STORM_FN code(CodeGen *state, CodeResult *boolResult);

			// Generate code to initialize the variable.
			virtual void STORM_FN initCode(CodeGen *state, LocalVar *var);

		private:
			// The Variant type.
			Type *variantType;
		};


		/**
		 * Weak cast from Maybe<T> to T.
		 */
		class WeakMaybeCast : public WeakCast {
			STORM_CLASS;
		public:
			// Create.
			STORM_CTOR WeakMaybeCast(Expr *expr);

			// Get a suitable position for the cast.
			virtual SrcPos STORM_FN pos();

			// Get variable we can write to.
			virtual MAYBE(Str *) STORM_FN overwrite();

			// Get the result type.
			virtual Value STORM_FN resultType();

			// Generate code.
			virtual void STORM_FN code(CodeGen *state, CodeResult *boolResult);

			// Generate code to initialize the variable.
			virtual void STORM_FN initCode(CodeGen *state, LocalVar *var);

		protected:
			// Output.
			virtual void STORM_FN toS(StrBuf *to) const;

		private:
			// Expression to cast.
			Expr *expr;

			// Variable used to store 'expr'.
			code::Var exprVar;

			// Type of 'exprVar'.
			Value exprType;

			// Generate code for class types.
			void classCode(MaybeClassType *c, CodeGen *state, CodeResult *boolResult);
			void classInit(MaybeClassType *c, CodeGen *state, LocalVar *var);

			// Genereate code for value types.
			void valueCode(MaybeValueType *c, CodeGen *state, CodeResult *boolResult);
			void valueInit(MaybeValueType *c, CodeGen *state, LocalVar *var);
		};


		// Figure out what kind of weak cast to create based on the type of the lhs of expressions like <lhs> as <type>.
		WeakCast *STORM_FN weakAsCast(Block *block, Expr *expr, SrcName *type);

	}
}
