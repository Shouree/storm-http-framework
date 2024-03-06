#pragma once
#include "Decl.h"
#include "Token.h"
#include "Core/Array.h"
#include "Compiler/Basic/Function.h"
#include "Compiler/Basic/Expr.h"
#include "Compiler/Basic/Block.h"

namespace storm {
	namespace syntax {
		STORM_PKG(lang.bnf);

		class Production;
		class ProductionType;

		/**
		 * Logic for generating code that extracts all children from a syntax tree into a plain
		 * array. Built on top of Basic Storm.
		 */
		class ChildrenFn : public bs::BSRawFn {
			STORM_CLASS;
		public:
			// Create.
			STORM_CTOR ChildrenFn(ProductionDecl *decl, ProductionType *owner, Rule *rule, Scope scope);

			// Create the body of the function.
			virtual bs::FnBody *STORM_FN createBody();

		private:
			// Scope.
			Scope scope;

			// The production we're extracting children from.
			Production *source;

			// Go through all tokens and add them to 'result'.
			void addTokens(bs::ExprBlock *block, bs::Expr *result);

			// Add a single token.
			void addToken(bs::ExprBlock *block, bs::Expr *result, Token *token);
			void addTokenIf(bs::ExprBlock *block, bs::Expr *result, Token *token);
			void addTokenLoop(bs::ExprBlock *block, bs::Expr *result, Nat from, Nat to);

			// Create 'result.push(tokenSrc)'.
			bs::Expr *push(bs::Block *block, bs::Expr *result, bs::Expr *tokenSrc);

			// Get 'this'.
			bs::Expr *thisExpr(bs::ExprBlock *block);

			// Treat the capture as a token:
			Nat tokenCount();
			Token *getToken(Nat pos);
		};


		// Create a children function.
		Function *STORM_FN createChildrenFn(ProductionDecl *option, ProductionType *owner, Scope scope);
	}
}
