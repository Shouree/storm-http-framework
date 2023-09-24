#pragma once
#include "Compiler/ExprResult.h"
#include "Core/SrcPos.h"
#include "Compiler/NamedThread.h"
#include "Compiler/CodeGen.h"
#include "Compiler/Syntax/SStr.h"
#include "Core/EnginePtr.h"

namespace storm {
	namespace bs {
		STORM_PKG(lang.bs);

		/**
		 * Base class for expressions (no difference between statements and expressions in this language!).
		 */
		class Expr : public ObjectOn<Compiler> {
			STORM_CLASS;
		public:
			STORM_CTOR Expr(SrcPos pos);

			// Where is this expression located in the source code?
			SrcPos pos;

			// Result of an expression. Default is void.
			virtual ExprResult STORM_FN result();

			// Generate code.
			// If "result" indicates that the expression returns a reference, you can ask for either
			// a reference or a value. If "result" returns a value, you can only ask for a value.
			// Some expressions support the other way around as well, but this can not be relied on.
			virtual void STORM_FN code(CodeGen *state, CodeResult *r);

			// Is it possible to cast this one expression to 'to'? < 0, no cast possible.
			virtual Int STORM_FN castPenalty(Value to);

			// Is it possible (and desirable) to isolate this expression in its own block? The
			// default implementation returns 'true' as we wish to isolate most expressions when we
			// put them inside an ExprBlock.
			virtual Bool STORM_FN isolate();

			// Get a position suitable as a source location. It sometimes differs from "pos" since
			// we in error messages typically want to point at details (e.g. a particular operator),
			// while in debugging information we want to include the entire range.
			virtual SrcPos STORM_FN largePos();

			// Is this an expression that is a "this"-variable? It may or may not be named "this",
			// but it is known to be referring to the same object as the current one, so we know
			// that we don't have to switch threads when calling member functions on the object this
			// variable is referring to.
			virtual Bool STORM_FN thisVariable() const;
		};


		/**
		 * Numeric literal.
		 */
		class NumLiteral : public Expr {
			STORM_CLASS;
		public:
			// Create, describes an integer.
			STORM_CTOR NumLiteral(SrcPos pos, Int i);
			STORM_CTOR NumLiteral(SrcPos pos, Long i);

			// Create, describes an unsigned number (will not allow signed output).
			STORM_CTOR NumLiteral(SrcPos pos, Word i);

			// Create, describes a floating-point number.
			STORM_CTOR NumLiteral(SrcPos pos, Double f);

			// Specify type using a suffix. 'suffix' is one of the characters 'binlwfd', which each
			// corresponds to the first letter in one of the primitive types.
			void STORM_FN setType(Str *suffix);

			// Return value.
			virtual ExprResult STORM_FN result();

			// Generate code.
			virtual void STORM_FN code(CodeGen *state, CodeResult *r);

			// Castable to?
			virtual Int STORM_FN castPenalty(Value to);

			// To string.
			virtual void STORM_FN toS(StrBuf *to) const;

		private:
			// Value (if integer).
			Long intValue;

			// Value (if float).
			Double floatValue;

			// Integer value?
			Bool isInt;

			// Unsigned number?
			Bool isSigned;

			// Specified type (if any).
			MAYBE(Type *) type;

			// Code for an int label.
			void intCode(CodeGen *state, CodeResult *r);

			// Code for a float label.
			void floatCode(CodeGen *state, CodeResult *r);
		};

		NumLiteral *STORM_FN intConstant(SrcPos pos, Str *str);
		NumLiteral *STORM_FN floatConstant(SrcPos pos, Str *str);
		NumLiteral *STORM_FN hexConstant(SrcPos pos, Str *str);

		/**
		 * String literal.
		 */
		class StrLiteral : public Expr {
			STORM_CLASS;
		public:
			// Create.
			STORM_CTOR StrLiteral(SrcPos pos, Str *str);

			// Return value.
			virtual ExprResult STORM_FN result();

			// Generate code.
			virtual void STORM_FN code(CodeGen *state, CodeResult *r);

			// To string.
			virtual void STORM_FN toS(StrBuf *to) const;

		private:
			// Value.
			Str *value;
		};

		StrLiteral *STORM_FN strConstant(syntax::SStr *str);
		StrLiteral *STORM_FN strConstant(SrcPos pos, Str *str);
		StrLiteral *STORM_FN rawStrConstant(SrcPos pos, Str *str);

		/**
		 * Boolean literal.
		 */
		class BoolLiteral : public Expr {
			STORM_CLASS;
		public:
			// Create.
			STORM_CTOR BoolLiteral(SrcPos pos, Bool value);

			// Return value.
			virtual ExprResult STORM_FN result();

			// Generate code.
			virtual void STORM_FN code(CodeGen *state, CodeResult *r);

			// To string.
			virtual void STORM_FN toS(StrBuf *to) const;

		private:
			// Value.
			Bool value;
		};

		/**
		 * Character literal.
		 */
		class CharLiteral : public Expr {
			STORM_CLASS;
		public:
			// Create.
			STORM_CTOR CharLiteral(SrcPos pos, Char value);

			// Return value.
			virtual ExprResult STORM_FN result();

			// Generate code.
			virtual void STORM_FN code(CodeGen *state, CodeResult *r);

			// To string.
			virtual void STORM_FN toS(StrBuf *to) const;

		private:
			// Value.
			Char value;
		};

		// Create a literal from a string.
		CharLiteral *STORM_FN charConstant(SrcPos pos, Str *value);


		/**
		 * Dummy expression, tells that we're returning a value of a specific type, but will not
		 * generate any code.
		 */
		class DummyExpr : public Expr {
			STORM_CLASS;
		public:
			STORM_CTOR DummyExpr(SrcPos pos, Value type);

			virtual ExprResult STORM_FN result();
			virtual void STORM_FN code(CodeGen *state, CodeResult *r);

		private:
			Value type;
		};

	}
}
