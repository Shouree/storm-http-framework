#pragma once
#include "Thread.h"
#include "Core/SrcPos.h"
#include "Value.h"
#include "Core/Array.h"
#include "Core/Set.h"

namespace storm {
	STORM_PKG(core.lang);

	/**
	 * The scope is divided into two parts. The first one, 'Scope', is a simple data type that holds
	 * a pointer to a leaf in the type tree as well as a pointer to the current 'ScopeLookup', which
	 * implements the lookup strategy.
	 */

	class Name;
	class NameLookup;
	class Named;
	class SrcName;
	class SimpleName;
	class Package;
	class Scope;

	/**
	 * The lookup strategy for a scope.
	 */
	class ScopeLookup : public ObjectOn<Compiler> {
		STORM_CLASS;
	public:
		// Create.
		STORM_CTOR ScopeLookup();

		// Create, give the name of 'void'.
		STORM_CTOR ScopeLookup(Str *voidName);
		ScopeLookup(const wchar *voidName);

		// Clone this lookup instance.
		virtual ScopeLookup *STORM_FN clone() const;

		// Find 'name' in 'in'.
		virtual MAYBE(Named *) STORM_FN find(Scope in, SimpleName *name);

		// Resolve 'name' to a type.
		virtual Value STORM_FN value(Scope in, SimpleName *name, SrcPos pos);

		/**
		 * Utility functions.
		 */

		// Find the first package when traversing parent() pointers.
		static Package *STORM_FN firstPkg(NameLookup *l);

		// Find the root package.
		static Package *STORM_FN rootPkg(Package *p);

		// Find the package 'core' from any source.
		static Package *STORM_FN corePkg(NameLookup *l);

		// Find the next candidate for the standard algorithm. (not counting 'core').
		static MAYBE(NameLookup *) STORM_FN nextCandidate(NameLookup *prev);

	private:
		// What is 'void' called in this language (if any)?
		Str *voidName;
	};

	// Find a Named from a SimpleName. 'scope' is used in visibility checks.
	MAYBE(Named *) STORM_FN find(Scope scope, NameLookup *root, SimpleName *name) ON(Compiler);

	/**
	 * Denotes a scope to use when looking up names. The scope itself is not much more than a policy
	 * along with the currently topmost element. For example, when looking for names relative a
	 * specific type, the type itself will be the topmost element. This is used to traverse the type
	 * hierarchy in any way the current implementation wishes to find a match for the name. This is
	 * designed so that the current implementation can be overridden by specific language
	 * implementations later on.
	 */
	class Scope {
		STORM_VALUE;
	public:
		// Create a scope that will never return anything.
		STORM_CTOR Scope();

		// Create the default lookup with a given topmost object.
		STORM_CTOR Scope(NameLookup *top);

		// Create a custom lookup.
		STORM_CTOR Scope(NameLookup *top, ScopeLookup *lookup);

		// Create a child scope.
		STORM_CTOR Scope(Scope parent, NameLookup *top);

		// Create a child scope.
		inline Scope STORM_FN child(NameLookup *top) const { return Scope(*this, top); }

		// Topmost object.
		MAYBE(NameLookup *) top;

		// Lookup object.
		MAYBE(ScopeLookup *) lookup;

		// Find the given NameRef, either by using an absolute path or something relative to the
		// current object.
		MAYBE(Named *) STORM_FN find(Name *name) const ON(Compiler);
		MAYBE(Named *) STORM_FN find(SimpleName *name) const ON(Compiler);
		MAYBE(Named *) find(const wchar *name, Array<Value> *params) const ON(Compiler);

		// Look up a value. Throws on error. Allows proper handling of void and type aliases.
		Value STORM_FN value(Name *name, SrcPos pos) const ON(Compiler);
		Value STORM_FN value(SrcName *name) const ON(Compiler);

		// Amend the scope with a node indicating that we're at a particular position.
		Scope STORM_FN withPos(SrcPos pos) const;

		// Deep copy.
		void STORM_FN deepCopy(CloneEnv *env);

		// Output.
		void STORM_FN toS(StrBuf *to) const;
	};

	// Get the root scope.
	Scope STORM_FN rootScope(EnginePtr e);

	// Output.
	wostream &operator <<(wostream &to, const Scope &scope);

	/**
	 * Lookup with extra top-level finders.
	 */
	class ScopeExtra : public ScopeLookup {
		STORM_CLASS;
	public:
		// Create.
		STORM_CTOR ScopeExtra();
		STORM_CTOR ScopeExtra(Str *voidName);

		// Clone.
		virtual ScopeLookup *STORM_FN clone() const;

		// Add an extra lookup to search.
		void STORM_FN addExtra(NameLookup *lookup);

		// Add an extra lookup to search, possibly ignoring exports.
		void STORM_FN addExtra(NameLookup *lookup, Bool useExports);

		// Get all lookups.
		Array<NameLookup *> *STORM_FN extra() const;

		// Find.
		virtual MAYBE(Named *) STORM_FN find(Scope in, SimpleName *name);

	private:
		// Additional NameLookups to search.
		Array<NameLookup *> *search;

		// Keep track of which objects are in 'search'.
		Set<TObject *> *inSearch;
	};

}
