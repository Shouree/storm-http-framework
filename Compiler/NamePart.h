#pragma once
#include "Core/Array.h"
#include "Value.h"
#include "Syntax/SStr.h"

namespace storm {
	STORM_PKG(core.lang);

	class SimplePart;
	class Scope;
	class Name;
	class Named;
	class NameOverloads;
	class NameLookup;

	/**
	 * Represents one part of a name. Each part is a string and zero or more parameters (think
	 * templates in C++). Parameters may either be resolved names, or other names that has not yet
	 * been resolved. The base class represents the case when parameters are not neccessarily resolved.
	 */
	class NamePart : public Object {
		STORM_CLASS;
	public:
		// Create with a single name.
		STORM_CTOR NamePart(Str *name);
		NamePart(const wchar *name);

		// Deep copy.
		virtual void STORM_FN deepCopy(CloneEnv *env);

		// Our name.
		Str *name;

		// Resolve names.
		virtual MAYBE(SimplePart *) find(const Scope &scope) ON(Compiler);

		// To string.
		virtual void STORM_FN toS(StrBuf *to) const;
	};

	/**
	 * A NamePart which has all parameters properly resolved to Values.
	 */
	class SimplePart : public NamePart {
		STORM_CLASS;
	public:
		// Create with just a name.
		STORM_CTOR SimplePart(Str *name);
		STORM_CTOR SimplePart(syntax::SStr *name);
		SimplePart(const wchar *name);

		// Create with name and parameters.
		STORM_CTOR SimplePart(Str *name, Array<Value> *params);

		// Create with name and one parameter.
		STORM_CTOR SimplePart(Str *name, Value param);

		// Deep copy.
		virtual void STORM_FN deepCopy(CloneEnv *env);

		// Parameters.
		Array<Value> *params;

		// Resolve names.
		virtual MAYBE(SimplePart *) find(const Scope &scope);

		// Compute the badness of a candidate. Returns -1 on no match, and otherwise a positive
		// measure of how bad the match is. The lookup process will pick the one with the lowest
		// badness. We also assume that an exact match has a badness of 0.
		virtual Int STORM_FN matches(Named *candidate, Scope source) const;

		// Present the lookup system with an alternative part. Parts are resolved in the order they
		// appear, and only the first one matches. This can be used to implement multiple levels of
		// name resolution rules without having to traverse the name tree multiple times (which is
		// both slow and causes confusing behavior in some situations).
		virtual MAYBE(SimplePart *) STORM_FN nextOption() const;

		// Note if a parameter is expected to alter the scope of searches. For example, the
		// expression a.b() is expected to look inside 'a' before looking inside other scopes. This
		// is not something the system respects by default. Rather, individual lookup
		// implementations may choose to respect it.
		virtual Bool STORM_FN scopeParam(Nat id) const;

		// Determine if a particular named entity should be visible. The default implementation
		// simply calls 'candidate->visibleFrom(source)', which is suitable in most cases. This
		// function is provided to allow overriding the default visibility rules.
		virtual Bool STORM_FN visible(Named *candidate, Scope source) const;

		// To string.
		virtual void STORM_FN toS(StrBuf *to) const;

		// Equals.
		virtual Bool STORM_FN operator ==(const SimplePart &o) const;

		// Hash.
		virtual Nat STORM_FN hash() const;
	};


	/**
	 * A NamePart which has unresolved (recursive) parameters.
	 * TODO: Support reference parameters.
	 */
	class RecPart : public NamePart {
		STORM_CLASS;
	public:
		// Create with just a name.
		STORM_CTOR RecPart(Str *name);
		STORM_CTOR RecPart(syntax::SStr *name);

		// Create with parameters as well.
		STORM_CTOR RecPart(Str *name, Array<Name *> *params);

		// Deep copy.
		virtual void STORM_FN deepCopy(CloneEnv *env);

		// Parameters.
		Array<Name *> *params;

		// Add a parameter.
		inline void STORM_FN add(Name *param) { params->push(param); }

		// Resolve.
		virtual MAYBE(SimplePart *) find(const Scope &scope);

		// To string.
		virtual void STORM_FN toS(StrBuf *to) const;
	};

}
