#pragma once
#include "Core/Array.h"
#include "Thread.h"
#include "NamedFlags.h"
#include "Value.h"
#include "Visibility.h"
#include "Scope.h"
#include "Doc.h"

namespace storm {
	STORM_PKG(core.lang);

	class Named;
	class NameSet;
	class SimpleName;
	class SimplePart;

	/**
	 * Interface for objects that can look up names.
	 */
	class NameLookup : public ObjectOn<Compiler> {
		STORM_CLASS;
	public:
		STORM_CTOR NameLookup();
		STORM_CTOR NameLookup(NameLookup *parent);

		// Find a named entity that corresponds to the specified `SimplePart` in this object.
		// Returns `null` if it is not found. The parameter `source` indicates in what context the
		// name lookup is performed, and it is used to determine the visibility of entities.
		virtual MAYBE(Named *) STORM_FN find(SimplePart *part, Scope source);

		// Convenience overloads for `find`.
		MAYBE(Named *) STORM_FN find(Str *name, Array<Value> *params, Scope source);
		MAYBE(Named *) STORM_FN find(Str *name, Value param, Scope source);
		MAYBE(Named *) STORM_FN find(Str *name, Scope source);
		MAYBE(Named *) find(const wchar *name, Array<Value> *params, Scope source);
		MAYBE(Named *) find(const wchar *name, Value param, Scope source);
		MAYBE(Named *) find(const wchar *name, Scope source);

		// Check if this entity has a particular parent, either directly or indirectly. If `parent`
		// is null, then false is always returned.
		Bool STORM_FN hasParent(MAYBE(NameLookup *) parent) const;

		// Get the parent object to this lookup, or `null` if none.
		virtual MAYBE(NameLookup *) STORM_FN parent() const;

		// Parent name lookup. This should be set by the parent. If it is `null`, the default
		// `parent` implementation asserts. Therefore, root objects need to override `parent` in
		// order to return `null`.
		MAYBE(NameLookup *) parentLookup;
	};


	/**
	 * An extension of NameLookup with a position.
	 *
	 * This is useful to build linked structures for a Scope in order to indicate that we're at a
	 * particular location, which is useful for name lookups etc.
	 */
	class LookupPos : public NameLookup {
		STORM_CLASS;
	public:
		// Create.
		STORM_CTOR LookupPos();
		STORM_CTOR LookupPos(SrcPos pos);

		// Our location in source code.
		SrcPos pos;
	};


	/**
	 * Denotes a named object in the compiler. Named objects are for example functions, types.
	 */
	class Named : public LookupPos {
		STORM_CLASS;
	public:
		// Create without parameters.
		STORM_CTOR Named(Str *name);
		STORM_CTOR Named(SrcPos pos, Str *name);

		// Create with parameters.
		STORM_CTOR Named(Str *name, Array<Value> *params);
		STORM_CTOR Named(SrcPos pos, Str *name, Array<Value> *params);

		// Our name. Note: may be null for a while during compiler startup.
		Str *name;

		// Our parameters. Note: may be null for a while during compiler startup.
		Array<Value> *params;

		// Visibility. 'null' means 'visible from everywhere'.
		MAYBE(Visibility *) visibility;

		// Documentation (if present).
		MAYBE(NamedDoc *) documentation;

		// Get a 'Doc' object for this entity. Uses 'documentation' if available, otherwise
		// generates a dummy object. This could be a fairly expensive operation, since all
		// documentation is generally *not* held in main memory.
		Doc *STORM_FN findDoc();

		// Check if this named entity is visible from 'source'.
		Bool STORM_FN visibleFrom(Scope source);
		Bool STORM_FN visibleFrom(MAYBE(NameLookup *) source);

		// Flags for this named object.
		NamedFlags flags;

		// Late initialization. Called twice during startup. Once after the system has advanced far
		// enough to use templates, and again when it has advanced far enough to use packages.
		virtual void lateInit();

		// Compute a path to this entity. Utilizes the parent pointers to traverse the name tree.
		// Will assert if not properly added to the name tree.
		SimpleName *STORM_FN path() const;

		// Compute a path to this entity. Ignores the case where some parent was not found, and
		// provides a truncated name in such cases. This is useful for messages, where it is not
		// important that the name can be used to find the same entity again.
		SimpleName *safePath() const;

		// Get a human-readable identifier for this named object. May return an incomplete
		// identifier if the entity is not properly added to the name tree.
		virtual Str *STORM_FN identifier() const;

		// Get a short version of the identifier. Only includes the name at this level with parameters.
		virtual Str *STORM_FN shortIdentifier() const;

		// Better asserts for 'parent'.
		virtual MAYBE(NameLookup *) STORM_FN parent() const;

		// Receive notifications from NameSet objects. (TODO: Move into separate class?)
		virtual void STORM_FN notifyAdded(NameSet *to, Named *added);

		// Receive notifications from NameSet objects. (TODO: Move into semarate class?)
		virtual void STORM_FN notifyRemoved(NameSet *to, Named *removed);

		// Force compilation of this entity (and any sub-objects contained in here).
		virtual void STORM_FN compile();

		// Discard references to source code stored in here. This is used to discard information
		// required to re-compile the entity from source to save memory. Packages and types do this
		// automatically unless instructed otherwise.
		virtual void STORM_FN discardSource();

		// String representation.
		virtual void STORM_FN toS(StrBuf *buf) const;

		// Output the visibility to a string buffer.
		void STORM_FN putVisibility(StrBuf *to) const;

	private:
		// Find closest named parent.
		MAYBE(Named *) closestNamed() const;
		MAYBE(Named *) safeClosestNamed() const;
	};

}
