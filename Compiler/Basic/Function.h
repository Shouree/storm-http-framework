#pragma once
#include "Compiler/NamedThread.h"
#include "Compiler/Function.h"
#include "Compiler/Syntax/SStr.h"
#include "Compiler/Syntax/Node.h"
#include "Compiler/Doc.h"
#include "Decl.h"
#include "Param.h"
#include "Block.h"

namespace storm {
	namespace bs {
		STORM_PKG(lang.bs);

		class FnBody;
		class BSFunction;

		/**
		 * Function declaration. This holds the needed information to create each function later
		 * on. Also acts as an intermediate store for types until we have found all types in the
		 * current package. Otherwise, we would behave like C, that the type declarations have to be
		 * before anything that uses them.
		 *
		 * If 'result' is null, then we assume that this function declares a setter function, since
		 * the return type is always 'null' for them.
		 */
		class FunctionDecl : public NamedDecl {
			STORM_CLASS;
		public:
			STORM_CTOR FunctionDecl(Scope scope,
									MAYBE(SrcName *) result,
									syntax::SStr *name,
									Array<NameParam> *params,
									syntax::Node *options,
									syntax::Node *body);

			// Values.
			Scope scope;
			syntax::SStr *name;
			MAYBE(SrcName *) result;
			Array<NameParam> *params;
			MAYBE(syntax::Node *) options;
			syntax::Node *body;

			// Temporary solution for updating a function.
			virtual MAYBE(Named *) STORM_FN update(Scope scope);
			void STORM_FN update(BSFunction *fn);

			// Get our name as a NamePart.
			NamePart *STORM_FN namePart() const;


			// To string.
			virtual void STORM_FN toS(StrBuf *to) const;

		private:
			// Create the actual function.
			virtual Named *STORM_FN doCreate();
		};

		// Declare setter functions.
		FunctionDecl *STORM_FN assignDecl(Scope scope,
										syntax::SStr *name,
										Array<NameParam> *params,
										syntax::Node *options,
										syntax::Node *body);


		/**
		 * Raw function that will not read syntax trees by parsing a string.
		 */
		class BSRawFn : public Function {
			STORM_CLASS;
		public:
			// Create.
			STORM_CTOR BSRawFn(Value result, syntax::SStr *name, Array<ValParam> *params, MAYBE(NamedThread *) thread);

			// Set the thread for this function. Should be done before code is generated for the
			// first time, and before someone else start depending on this function.
			void STORM_FN thread(Scope scope, SrcName *name);

			// Override this to create the syntax tree to compile. Expected to work until 'clearBody' is called.
			// TODO: Mark as abstract.
			virtual FnBody *STORM_FN createBody();

			// Called when we know we don't need the body anymore, i.e. 'createBody' may stop
			// returning sensible results.
			virtual void STORM_FN clearBody();

			// Make this function static by removing the 'this' parameter. Only do this before the
			// function is added to the name tree!
			void STORM_FN makeStatic();

			// Make the function inlineable. Only has affect before the function is compiled.
			void STORM_FN makeInline();

			// Add function parameters to a block. Mainly for internal use.
			Array<LocalVar *> *STORM_FN addParams(Block *block);

			// Get parameters as required by documentation.
			Array<DocParam> *STORM_FN docParams();

		protected:
			// Parameter names and values.
			Array<ValParam> *valParams;

			// Re-compile at next execution.
			void STORM_FN reset();

			// Override if you want to create a CodeGen object yourself.
			virtual CodeGen *STORM_FN createRawBody();

		private:
			// Generate code.
			CodeGen *CODECALL generateCode();

			// Generate code for inline usage.
			void CODECALL generateInlineCode(InlineParams params);

			// Initialize.
			void init(NamedThread *thread);

			// Remember if we have cleared body data.
			Bool bodyCleared;

			// Should we be an inline function?
			Bool isInline;
		};


		/**
		 * Function in the BS language.
		 */
		class BSFunction : public BSRawFn {
			STORM_CLASS;
		public:
			// Create a function.
			STORM_CTOR BSFunction(Value result, syntax::SStr *name, Array<ValParam> *params, Scope scope,
								MAYBE(NamedThread *) thread, syntax::Node *body);

			// Scope.
			Scope scope;

			// Code.
			MAYBE(syntax::Node *) body;

			// Create the body from our string.
			virtual FnBody *STORM_FN createBody();
			virtual void STORM_FN clearBody();

			// Temporary solution for updating a function.
			Bool STORM_FN update(Array<ValParam> *params, syntax::Node *body, SrcPos pos);
			Bool STORM_FN update(Array<ValParam> *params, syntax::Node *body);
			Bool STORM_FN update(Array<Str *> *params, syntax::Node *body);
			Bool STORM_FN update(BSFunction *from);
		};


		/**
		 * Function using a pre-created syntax tree.
		 */
		class BSTreeFn : public BSRawFn {
			STORM_CLASS;
		public:
			// Create.
			STORM_CTOR BSTreeFn(Value result, syntax::SStr *name, Array<ValParam> *params, MAYBE(NamedThread *) thread);

			// Set the root of the syntax tree. Resetting the body also makes the function re-compile.
			void STORM_ASSIGN body(FnBody *body);

			// Override to use the body.
			virtual FnBody *STORM_FN createBody();
			virtual void STORM_FN clearBody();

		private:
			// Body.
			MAYBE(FnBody *) root;
		};


		/**
		 * Abstract function that throws an exception when invoked.
		 */
		class BSAbstractFn : public BSRawFn {
			STORM_CLASS;
		public:
			STORM_CTOR BSAbstractFn(Value result, syntax::SStr *name, Array<ValParam> *params);

		protected:
			// Override to use the body.
			virtual CodeGen *STORM_FN createRawBody();
		};


		/**
		 * Body of a function.
		 */
		class FnBody : public ExprBlock {
			STORM_CLASS;
		public:
			STORM_CTOR FnBody(BSRawFn *owner, Scope scope);
			STORM_CTOR FnBody(BSFunction *owner);

			// Store the result type (needed for 'return' among others).
			Value type;

			// Parameters, ordered as they appear in the list of formal parameters.
			Array<LocalVar *> *parameters;

			// If present, we are generating code in an inline function. That means that the result
			// should be stored inside this CodeResult, and execution should jump to the label
			// indicated below.
			MAYBE(CodeResult *) inlineResult;

			// Label to jump to when we return from an inline function.
			code::Label inlineLabel;

			// Block to jump out to.
			code::Block inlineBlock;

			// We don't need to create a separate block for the function body itself, we can just
			// use the root block.
			virtual void STORM_FN code(CodeGen *state, CodeResult *to);

		};

	}
}
