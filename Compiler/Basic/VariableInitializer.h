#pragma once
#include "Function.h"

namespace storm {
	namespace bs {
		STORM_PKG(lang.bs);

		/**
		 * Initializer function for variables. Used for both global variables and member variables.
		 */
		class VariableInitializer : public BSRawFn {
			STORM_CLASS;
		public:
			// Create. Note: the node is expected to be a lang.bs.SExpr.
			STORM_CTOR VariableInitializer(SrcPos pos, Value type, Scope scope, MAYBE(syntax::Node *) initExpr);

			// Generate the body.
			virtual FnBody *STORM_FN createBody();

		private:
			// Location.
			SrcPos pos;

			// Scope.
			Scope scope;

			// Initialization expression.
			MAYBE(syntax::Node *) initExpr;
		};

	}
}
