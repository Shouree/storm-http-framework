#include "stdafx.h"
#include "Lookup.h"
#include "Package.h"
#include "Engine.h"
#include "Compiler/Syntax/Reader.h"

namespace storm {
	namespace bs {

		BSLookup::BSLookup() : ScopeLookup(S("void")) {
			toInclude = new (this) Array<Package *>();
			inIncludes = new (this) Set<TObject *>();
		}

		BSLookup::BSLookup(Array<Package *> *inc) : ScopeLookup(S("void")) {
			toInclude = new (this) Array<Package *>();
			inIncludes = new (this) Set<TObject *>();
			for (Nat i = 0; i < inc->count(); i++)
				addInclude(inc->at(i), true);
		}

		ScopeLookup *BSLookup::clone() const {
			BSLookup *copy = new (this) BSLookup();
			copy->toInclude->append(toInclude);
			copy->inIncludes = new (this) Set<TObject *>(*inIncludes);
			return copy;
		}

		void BSLookup::addInclude(Package *p) {
			addInclude(p, true);
		}

		void BSLookup::addInclude(Package *p, Bool useExports) {
			if (inIncludes->has(p))
				return;

			inIncludes->put(p);
			toInclude->push(p);

			if (useExports) {
				Array<Package *> *add = p->exports();
				for (Nat i = 0; i < add->count(); i++)
					addInclude(add->at(i), true);
			}
		}

		Array<Package *> *BSLookup::includes() const {
			return new (this) Array<Package *>(*toInclude);
		}

		static Named *findFirstParam(Scope s, SimpleName *name, Bool scope) {
			if (name->count() != 1)
				return null;

			for (SimplePart *last = name->last(); last; last = last->nextOption()) {
				if (!last->params->any() || last->scopeParam(0) != scope)
					continue;

				Value firstParam = last->params->at(0);
				if (!firstParam.type)
					continue;

				if (Named *r = firstParam.type->find(last, s))
					return r;

				// TODO: Also look in the parent scope of the last type? This will fix issues
				// with 2 / 0.2 etc.
			}

			return null;
		}

		Named *BSLookup::find(Scope s, SimpleName *name) {
			// 1: If the first parameter is marked as scope-altering, prioritize lookups there.
			if (Named *found = findFirstParam(s, name, true))
				return found;

			// 2: Traverse until we reach the root.
			for (NameLookup *at = s.top; at; at = ScopeLookup::nextCandidate(at)) {
				if (Named *found = storm::find(s, at, name))
					return found;
			}

			// 3: Now we are at a location where we can start looking for more "complex" things. At
			// this point, we try to examine the first parameters if the name only contains of a
			// single part. This is since users expect x.foo() to examine the scope of 'x', and
			// since this is equivalent to foo(x).
			if (Named *found = findFirstParam(s, name, false))
				return found;

			// 4: Look in the root package.
			if (Named *found = storm::find(s, engine().corePackage(), name))
				return found;

			// 5: Lastly, examine imported packages.
			for (Nat i = 0; i < toInclude->count(); i++) {
				if (Named *found = storm::find(s, toInclude->at(i), name))
					return found;
			}

			return null;
		}

		void BSLookup::addSyntax(Scope from, syntax::ParserBase *to) {
			// Note: We have already considered exports, so we ask the syntax pkg to not look at exports either.

			// Current package.
			to->addSyntax(firstPkg(from.top), false);

			for (Nat i = 0; i < toInclude->count(); i++)
				to->addSyntax(toInclude->at(i), false);
		}

		Bool addInclude(Scope to, Package *pkg) {
			if (BSLookup *s = as<BSLookup>(to.lookup)) {
				s->addInclude(pkg);
				return true;
			} else {
				WARNING(L"This is not what you want to do!");
				return false;
			}
		}

		void addSyntax(Scope scope, syntax::ParserBase *to) {
			if (BSLookup *s = as<BSLookup>(scope.lookup)) {
				s->addSyntax(scope, to);
			} else {
				WARNING(L"This is probably not what you want to do!");
			}
		}

	}
}
