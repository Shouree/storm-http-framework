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

			// Add included package, taking exports into account.
			void STORM_FN addInclude(Package *p);

			// Add included package, possibly ignoring exports.
			void STORM_FN addInclude(Package *p, Bool useExports);

			// Get all included packages.
			Array<Package *> *STORM_FN includes() const;

			// Find things.
			virtual MAYBE(Named *) STORM_FN find(Scope in, SimpleName *name);

			// Add syntax to a parser.
			void STORM_FN addSyntax(Scope from, syntax::ParserBase *to);

		private:
			// Included packages.
			Array<Package *> *toInclude;

			// Packages that are in 'includes'.
			Set<TObject *> *inIncludes;
		};

		// Add includes. Note: it is generally more efficient to add multiple includes at once.
		Bool STORM_FN addInclude(Scope to, Package *p);

		// Add syntax to a parser.
		void STORM_FN addSyntax(Scope from, syntax::ParserBase *to);


	}
}
