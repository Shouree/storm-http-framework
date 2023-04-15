#pragma once
#include "Decl.h"
#include "Compiler/Scope.h"
#include "Compiler/Name.h"
#include "Compiler/Visibility.h"
#include "Compiler/Function.h"
#include "Compiler/Syntax/Node.h"

namespace storm {
	namespace bs {
		STORM_PKG(lang.bs);

		/**
		 * Global variable declaration.
		 */
		class GlobalVarDecl : public NamedDecl {
			STORM_CLASS;
		public:
			// Create.
			STORM_CTOR GlobalVarDecl(Scope env, SrcName *type, syntax::SStr *name, SrcName *thread);

			// Scope.
			Scope scope;

			// Type of the variable.
			SrcName *type;

			// Name of the variable.
			syntax::SStr *name;

			// Thread associated with the global variable.
			SrcName *thread;

			// Initialize to.
			MAYBE(syntax::Node *) initExpr;

			// Initialize to an expression.
			void STORM_FN init(syntax::Node *initExpr);

			// To string.
			virtual void STORM_FN toS(StrBuf *to) const;

		protected:
			// Create actual entities.
			virtual Named *STORM_FN doCreate();

			// Resolve the variable.
			virtual void STORM_FN doResolve(Named *entity);

		private:
			// Create an initializer for the variable.
			Function *createInitializer(Value type, Scope scope, NamedThread *thread);
		};

	}
}
