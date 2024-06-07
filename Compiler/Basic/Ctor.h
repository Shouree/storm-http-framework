#pragma once
#include "Core/Map.h"
#include "Core/Set.h"
#include "Core/Array.h"
#include "Compiler/Function.h"
#include "Compiler/Syntax/SStr.h"
#include "Compiler/Syntax/Node.h"
#include "Compiler/Variable.h"
#include "Block.h"
#include "Param.h"

namespace storm {
	namespace bs {
		STORM_PKG(lang.bs);

		class CtorBody;
		class Class;

		/**
		 * Raw constructor call, much like BSRawFn.
		 */
		class BSRawCtor : public Function {
			STORM_CLASS;
		public:
			// Create.
			STORM_CTOR BSRawCtor(Array<ValParam> *params, SrcPos pos);

			// We may need to run on a specific thread based on the current actual parameters.
			virtual code::Var STORM_FN findThread(CodeGen *s, Array<code::Operand> *params);

			// Add parameters. Outputs 'threadParam' if not null, and remaining parameters for positional access.
			void addParams(Block *to, LocalVar *&threadParam, Array<LocalVar *> *&params);

			// Create the body. Expected to work until 'clearBody' is called.
			virtual CtorBody *STORM_FN createBody();

			// Called when we know we don't need the body anymore, i.e. 'createBody' may stop
			// returning sensible results.
			virtual void STORM_FN clearBody();

		protected:
			// Re-compile at next execution.
			void STORM_FN reset();

		private:
			// Parameter names.
			Array<ValParam> *params;

			// Generate code.
			CodeGen *CODECALL generateCode();

			// Do we need a 'hidden' thread parameter?
			bool needsThread;
		};


		/**
		 * A constructor. Enforces that the parent constructor is called.
		 */
		class BSCtor : public BSRawCtor {
			STORM_CLASS;
		public:
			// Create. If body is null, we will generate the default ctor.
			BSCtor(Array<ValParam> *params, Scope scope, MAYBE(syntax::Node *) body, SrcPos pos);

			// Scope.
			Scope scope;

			// Body.
			MAYBE(syntax::Node *) body;

			// Create the body.
			virtual CtorBody *STORM_FN createBody();
			virtual void STORM_FN clearBody();

		private:
			// Default ctor body.
			CtorBody *defaultParse();
		};


		/**
		 * A constructor using a pre-created syntax tree.
		 */
		class BSTreeCtor : public BSRawCtor {
			STORM_CLASS;
		public:
			// Create.
			STORM_CTOR BSTreeCtor(Array<ValParam> *params, SrcPos pos);

			// Set the root of the syntax tree. Resetting the body also makes the function re-compile.
			void STORM_ASSIGN body(CtorBody *body);

			// Create the body.
			virtual CtorBody *STORM_FN createBody();
			virtual void STORM_FN clearBody();

		private:
			// Body.
			MAYBE(CtorBody *) root;
		};


		/**
		 * Body of the constructor.
		 */
		class CtorBody : public ExprBlock {
			STORM_CLASS;
		public:
			STORM_CTOR CtorBody(BSCtor *owner);
			STORM_CTOR CtorBody(BSRawCtor *owner, Scope scope);

			// Owning function.
			Function *owner;

			// Temporary storage of the actual LocalVar that stores the parameter we need to capture.
			LocalVar *threadParam;

			// Other formal parameters, in the order they appear in the formal parameter list.
			Array<LocalVar *> *parameters;

			// Stores a copy of the parameter used to store the thread. Not reference counted.
			// This is to make it impossible to overwrite the thread parameter before passing it
			// to the TObject's constructor, like this:
			// ctor(Thread a) { a = x; init(); }.
			code::Var thread;

			// Is a call to the super class' constructor present?
			Bool superCalled;

			// Is an initialization block present?
			Bool initDone;

			// Make sure we're properly initialized.
			void STORM_FN checkInit();

			// We don't need to create a separate block, we can just use the root block instead.
			virtual void STORM_FN code(CodeGen *state, CodeResult *to);

		};

		/**
		 * Call the constructor of the super class.
		 *
		 * After this, 'this' will be available in the function, but the type will be that of the
		 * super class, not the current class. That only happens after the current class is properly
		 * initialized by using InitBlock.
		 */
		class SuperCall : public Expr {
			STORM_CLASS;
		public:
			// Create.
			STORM_CTOR SuperCall(SrcPos pos, CtorBody *block, Actuals *params);

			// Result (always void).
			virtual ExprResult STORM_FN result();

			// Code.
			virtual void STORM_FN code(CodeGen *s, CodeResult *r);

			// To string.
			virtual void STORM_FN toS(StrBuf *to) const;

		private:
			// Parameters.
			Actuals *params;

			// The variable for the 'this' pointer.
			LocalVar *thisVar;

			// Our type.
			Value thisPtr;

			// Scope.
			CtorBody *block;
		};

		/**
		 * Initializer.
		 */
		class Initializer : public Object {
			STORM_CLASS;
		public:
			// Assignment initialization.
			STORM_CTOR Initializer(syntax::SStr *name, Expr *expr);

			// Custom constructor.
			STORM_CTOR Initializer(syntax::SStr *name, Actuals *params);

			// Name of the variable.
			syntax::SStr *name;

			// Initialize by assignment (may be null).
			MAYBE(Expr *) expr;

			// Initialize to (may be null).
			MAYBE(Actuals *) params;

			// Output.
			virtual void STORM_FN toS(StrBuf *to) const;
		};

		/**
		 * Initialize the class, optionally also calling the constructor of the super class.
		 *
		 * After this, 'this' will be available and of the proper type.
		 */
		class InitBlock : public Expr {
			STORM_CLASS;
		public:
			// Create. Missing 'params' means that no parameters were provided.
			STORM_CTOR InitBlock(SrcPos pos, CtorBody *block, MAYBE(Actuals *) params);
			STORM_CTOR InitBlock(SrcPos pos, CtorBody *block, MAYBE(Actuals *) params, Array<Initializer *> *init);

			// Add a new initializer.
			void STORM_FN init(Initializer *init);

			// Result.
			virtual ExprResult STORM_FN result();

			// Code.
			virtual void STORM_FN code(CodeGen *s, CodeResult *r);

			// To string.
			virtual void STORM_FN toS(StrBuf *to) const;

			// Don't isolate the initializer, that will break things.
			virtual Bool STORM_FN isolate();

		private:
			// Member of.
			Value thisPtr;

			// This parameter.
			LocalVar *thisVar;

			// Scope.
			Scope scope;

			// Initializers. Remember the order so that we can initialize members in the order they
			// appear in the source code.
			Array<Initializer *> *initializers;

			// Remember which variables have initializers.
			Set<Str *> *initialized;

			// Init.
			void init(CtorBody *block, MAYBE(Actuals *) params);

			// Initialize a variable.
			void initVar(CodeGen *s, MemberVar *var, Initializer *to);
			void initVarCtor(CodeGen *s, MemberVar *var, Actuals *to, Bool explicitCast);
			void initVarAssign(CodeGen *s, MemberVar *var, Expr *to);

			// Initialize a variable with its default constructor.
			void initVarDefault(CodeGen *s, MemberVar *var);

			// Initialize a variable with its default initializer.
			void initVarInitializer(CodeGen *s, MemberVar *var);
		};


		/**
		 * Delegate initialization to another constructor. Can not be used together with separate
		 * 'super' or 'init' calls.
		 */
		class DelegateCall : public Expr {
			STORM_CLASS;
		public:
			// Create.
			STORM_CTOR DelegateCall(SrcPos pos, CtorBody *block, Actuals *params);

			// Result.
			virtual ExprResult STORM_FN result();

			// Code.
			virtual void STORM_FN code(CodeGen *s, CodeResult *r);

			// To string.
			virtual void STORM_FN toS(StrBuf *to) const;

			// Don't isolate the delegate call.
			virtual Bool STORM_FN isolate();

		private:
			// Member of.
			Value thisPtr;

			// This parameter.
			LocalVar *thisVar;

			// Block.
			CtorBody *block;

			// Actual parameters to the other constructor.
			Actuals *params;
		};

	}
}
