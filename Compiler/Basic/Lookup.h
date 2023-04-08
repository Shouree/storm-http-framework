#pragma once
#include "Compiler/Syntax/Parser.h"
#include "Compiler/Scope.h"

namespace storm {
	namespace bs {
		STORM_PKG(lang.bs);

		class BSLookup : public ScopeLookup {
			STORM_CLASS;
		public:
			// Create, optionally specify includes.
			STORM_CTOR BSLookup();
			STORM_CTOR BSLookup(Array<Package *> *includes);

			// Clone.
			virtual ScopeLookup *STORM_FN clone() const;

			// Included packages.
			Array<Package *> *includes;

			// Find things.
			virtual MAYBE(Named *) STORM_FN find(Scope in, SimpleName *name);

			// Add syntax to a parser.
			void STORM_FN addSyntax(Scope from, syntax::ParserBase *to);
		};

		// Add includes. Note: it is generally more efficient to add multiple includes at once.
		Bool STORM_FN addInclude(Scope to, Package *p);

		// Add syntax to a parser.
		void STORM_FN addSyntax(Scope from, syntax::ParserBase *to);


	}
}
