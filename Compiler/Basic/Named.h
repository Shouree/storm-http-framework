#pragma once
#include "Expr.h"
#include "Var.h"
#include "Actuals.h"
#include "Compiler/Function.h"
#include "Compiler/NamedThread.h"
#include "Variable.h"

namespace storm {
	namespace bs {
		STORM_PKG(lang.bs);

		class UnresolvedName;

		/**
		 * Execute a function.
		 */
		class FnCall : public Expr {
			STORM_CLASS;
			friend class AsyncFnCall;
		public:
			// if '!lookup', then the lookup functionality (such as vtables) will not be used.
			STORM_CTOR FnCall(SrcPos pos, Scope scope, Function *toExecute, Actuals *params);
			STORM_CTOR FnCall(SrcPos pos, Scope scope, Function *toExecute, Actuals *params, Bool lookup);

			// Create a UnresolvedName object describing how to find this function.
			UnresolvedName *STORM_FN name();

			// Get the function we are going to call.
			inline Function *STORM_FN function() const { return toExecute; }

			// Result type.
			virtual ExprResult STORM_FN result();

			// Generate code.
			virtual void STORM_FN code(CodeGen *s, CodeResult *to);

			// Location.
			virtual SrcPos STORM_FN largePos();

		protected:
			// To string.
			virtual void STORM_FN toS(StrBuf *to) const;

		private:
			// Function to run.
			Function *toExecute;

			// Parameters.
			Actuals *params;

			// Scope this function is called within.
			Scope scope;

			// Use lookup?
			Bool lookup;
		};


		/**
		 * Explicitly async function call.
		 */
		class AsyncFnCall : public FnCall {
			STORM_CLASS;
		public:
			// Create from a FnCall. Make it async without specifying a thread explicitly.
			STORM_CTOR AsyncFnCall(FnCall *original);

			// Create from a FnCall, the thread was specified explicitly.
			STORM_CTOR AsyncFnCall(FnCall *original, Expr *thread);

			// Result type.
			virtual ExprResult STORM_FN result();

			// Generate code.
			virtual void STORM_FN code(CodeGen *s, CodeResult *to);

		protected:
			// To string.
			virtual void STORM_FN toS(StrBuf *to) const;

		private:
			// The thread specified, if any.
			Expr *thread;
		};


		/**
		 * Execute a constructor.
		 */
		class CtorCall : public Expr {
			STORM_CLASS;
		public:
			// Call a constructor.
			STORM_CTOR CtorCall(SrcPos pos, Scope scope, Function *ctor, Actuals *params);

			// Result type.
			virtual ExprResult STORM_FN result();

			// Generate code.
			virtual void STORM_FN code(CodeGen *s, CodeResult *to);

			// Location.
			virtual SrcPos STORM_FN largePos();

		protected:
			// To string.
			virtual void STORM_FN toS(StrBuf *to) const;

		private:
			// Type to create.
			Value toCreate;

			// Constructor to run.
			Function *ctor;

			// Parameters.
			Actuals *params;

			// Scope we're being executed in.
			Scope scope;

			// Create a value.
			void createValue(CodeGen *s, CodeResult *to);

			// Create a class.
			void createClass(CodeGen *s, CodeResult *to);
		};

		// Call the default constructor for T.
		CtorCall *STORM_FN defaultCtor(const SrcPos &pos, Scope scope, Type *t);

		// Call the copy-constructor for T.
		CtorCall *STORM_FN copyCtor(const SrcPos &pos, Scope scope, Type *t, Expr *src);


		/**
		 * Get a local variable.
		 */
		class LocalVarAccess : public Expr {
			STORM_CLASS;
		public:
			STORM_CTOR LocalVarAccess(SrcPos pos, LocalVar *var);

			// Result type.
			virtual ExprResult STORM_FN result();

			// Generate code.
			virtual void STORM_FN code(CodeGen *s, CodeResult *to);

			// Variable to access.
			LocalVar *var;

			// Is this a known "this"-variable?
			Bool STORM_FN thisVariable() const {
				return var->thisVariable();
			}

		protected:
			// To string.
			virtual void STORM_FN toS(StrBuf *to) const;
		};


		/**
		 * Get a local variable (bare-bones wrapper for interfacing with other languages).
		 */
		class BareVarAccess : public Expr {
			STORM_CLASS;
		public:
			STORM_CTOR BareVarAccess(SrcPos pos, Value type, code::Var var);

			// Result.
			virtual ExprResult STORM_FN result();

			// Generate code.
			virtual void STORM_FN code(CodeGen *s, CodeResult *to);

			// Type and variable.
			Value type;
			code::Var var;

		protected:
			// To string.
			virtual void STORM_FN toS(StrBuf *to) const;
		};


		/**
		 * Read a type variable.
		 */
		class MemberVarAccess : public Expr {
			STORM_CLASS;
		public:
			// Standard creation if you don't need to take the case where "member" might not have
			// been specified by the user into account (e.g. when you generate code internally).
			STORM_CTOR MemberVarAccess(SrcPos pos, Expr *member, MemberVar *var);

			// If "implicitMember" is true, then the expression "member" was implicitly
			// generated. This mostly affects the decision of whether or not this is a "plain"
			// variable in weak casts.
			STORM_CTOR MemberVarAccess(SrcPos pos, Expr *member, MemberVar *var, Bool implicitMember);

			// Result type.
			virtual ExprResult STORM_FN result();

			// Generate code.
			virtual void STORM_FN code(CodeGen *s, CodeResult *to);

			// Notify that we will try to assign to this variable. This will not affect the
			// correctness of this note, but failing to call 'assignTarget' when other code will
			// attempt to modify the returned reference will result in no error message to the user.
			void STORM_FN assignResult();

			// Variable to access.
			MemberVar *var;

			// Location.
			virtual SrcPos STORM_FN largePos();

			// Is 'member' the implicit 'this' pointer?
			Bool implicitMember() const { return implicit; }

		protected:
			// Output.
			virtual void STORM_FN toS(StrBuf *to) const;

		private:
			// Member in which the variable is.
			Expr *member;

			// Will we be part of an assignment? This is only used to generate better error
			// messages. Nothing else.
			Bool assignTo;

			// Did "member" get specified implicitly?
			Bool implicit;

			// Generate code for a value access.
			void valueCode(CodeGen *s, CodeResult *to);

			// Generate code for a class access.
			void classCode(CodeGen *s, CodeResult *to);

			// Extract the value itself from 'ptrA'
			void extractCode(CodeGen *s, CodeResult *to);

			// Extract the value, assuming we don't need a deep copy.
			void extractPlainCode(CodeGen *s, CodeResult *to);

			// Extract the value, assuming we need a deep copy.
			void extractCopyCode(CodeGen *s, CodeResult *to);
		};


		/**
		 * Read a global variable.
		 */
		class GlobalVarAccess : public Expr {
			STORM_CLASS;
		public:
			STORM_CTOR GlobalVarAccess(SrcPos pos, GlobalVar *var);

			// Member to access.
			GlobalVar *var;

			// Result type.
			virtual ExprResult STORM_FN result();

			// Generate code.
			virtual void STORM_FN code(CodeGen *s, CodeResult *to);

			// To string.
			virtual void STORM_FN toS(StrBuf *to) const;
		};


		/**
		 * Read a named thread variable.
		 */
		class NamedThreadAccess : public Expr {
			STORM_CLASS;
		public:
			// Create.
			STORM_CTOR NamedThreadAccess(SrcPos pos, NamedThread *thread);

			// Result type.
			virtual ExprResult STORM_FN result();

			// Generate code.
			virtual void STORM_FN code(CodeGen *s, CodeResult *to);

		protected:
			// Output.
			virtual void STORM_FN toS(StrBuf *to) const;

		private:
			// Which thread?
			NamedThread *thread;
		};


		/**
		 * Default assignment operator.
		 */
		class ClassAssign : public Expr {
			STORM_CLASS;
		public:
			STORM_CTOR ClassAssign(Expr *to, Expr *value, Scope scope);

			// Result type.
			virtual ExprResult STORM_FN result();

			// Generate code.
			virtual void STORM_FN code(CodeGen *s, CodeResult *to);

		protected:
			// Output.
			virtual void STORM_FN toS(StrBuf *to) const;

		private:
			Expr *to;
			Expr *value;
		};


		/**
		 * Default compare for classes.
		 */
		class ClassCompare : public Expr {
			STORM_CLASS;
		public:
			// Create. Optionally negate the result (for !=).
			STORM_CTOR ClassCompare(SrcPos pos, Expr *lhs, Expr *rhs, Bool negate);

			// Result type.
			virtual ExprResult STORM_FN result();

			// Generate code.
			virtual void STORM_FN code(CodeGen *s, CodeResult *to);

		protected:
			// Output.
			virtual void STORM_FN toS(StrBuf *to) const;

		private:
			Expr *lhs;
			Expr *rhs;
			Bool negate;
		};


		// Make a regular function call return a future to the result instead of waiting for the result
		// directly. Syntactically, this is made by adding 'spawn' to it.
		Expr *STORM_FN spawnExpr(Expr *expr);

		// Make a regular function call return a future to the result instead of waiting for the
		// result directly. Additionally, attempt to execute it on the specified thread.
		Expr *STORM_FN spawnOnExpr(Expr *thread, Expr *expr);

	}
}
