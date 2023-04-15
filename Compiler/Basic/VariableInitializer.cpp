#include "stdafx.h"
#include "VariableInitializer.h"
#include "Resolve.h"
#include "Named.h"

namespace storm {
	namespace bs {

		VariableInitializer::VariableInitializer(SrcPos pos, Value type, Scope scope, MAYBE(syntax::Node *) initExpr)
			: BSRawFn(type, new (engine()) syntax::SStr(S("init")), new (engine()) Array<ValParam>(), null),
			  pos(pos), scope(scope), initExpr(initExpr) {}

		FnBody *VariableInitializer::createBody() {
			FnBody *body = new (this) FnBody(this, scope);

			if (initExpr) {
				// Use the expression.
				body->add(syntax::transformNode<Expr, Block *>(initExpr, body));
			} else {
				// Create an instance using the default constructor.
				body->add(defaultCtor(pos, scope, result.type));
			}

			return body;
		}

	}
}
