#include "stdafx.h"
#include "Decl.h"
#include "Doc.h"
#include "Compiler/Function.h"
#include "Compiler/Exception.h"

namespace storm {
	namespace bs {

		NamedDecl::NamedDecl() : visibility(null), thread(null), created(null) {}

		Named *NamedDecl::create() {
			if (created)
				return created;

			created = doCreate();
			created->visibility = visibility;

			// Apply documentation to functions, even if no comment is given. If not, functions
			// without parameters will not show parameter names in the documentation.
			if (docPos.any() || as<Function>(created))
				applyDoc(docPos, created);

			return created;
		}

		void NamedDecl::resolve() {
			if (created)
				doResolve(created);
		}

		MAYBE(Named *) NamedDecl::update(Scope scope) {
			// Not supported.
			return null;
		}

		void NamedDecl::doResolve(Named *entity) {}

	}
}
