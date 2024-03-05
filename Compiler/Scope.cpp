#include "stdafx.h"
#include "Scope.h"
#include "Name.h"
#include "Named.h"
#include "Package.h"
#include "Type.h"
#include "Engine.h"
#include "Exception.h"
#include "Core/Str.h"
#include "Core/StrBuf.h"

namespace storm {

	Named *find(Scope scope, NameLookup *root, SimpleName *name) {
		if (name->empty())
			return as<Named>(root);

		Named *at = root->find(name->at(0), scope);
		for (nat i = 1; at != null && i < name->count(); i++) {
			at = at->find(name->at(i), scope);
		}

		return at;
	}

	/**
	 * ScopeLookup.
	 */

	ScopeLookup::ScopeLookup() {}

	ScopeLookup::ScopeLookup(Str *v) : voidName(v) {}

	ScopeLookup::ScopeLookup(const wchar *s) {
		voidName = new (this) Str(s);
	}

	ScopeLookup *ScopeLookup::clone() const {
		return new (this) ScopeLookup(voidName);
	}

	Package *ScopeLookup::firstPkg(NameLookup *l) {
		NameLookup *now = l;
		while (!as<Package>(now)) {
			now = now->parent();
			if (!now)
				throw new (l) InternalError(TO_S(l, S("Unable to find a package when traversing parents of ") << l));
		}
		return as<Package>(now);
	}

	Package *ScopeLookup::rootPkg(Package *p) {
		return p->engine().package();

		// NameLookup *at = p;
		// while (at->parent())
		// 	at = at->parent();

		// assert(as<Package>(at), L"Detached NameLookup found!");
		// return as<Package>(at);
	}

	Package *ScopeLookup::corePkg(NameLookup *l) {
		return l->engine().corePackage();

		// Package *top = firstPkg(l);
		// Package *root = rootPkg(top);
		// SimplePart *core = new (l) SimplePart(new (l) Str(L"core"));
		// return as<Package>(root->find(core, Scope(root)));
	}

	NameLookup *ScopeLookup::nextCandidate(NameLookup *prev) {
		if (Package *pkg = as<Package>(prev)) {
			// Examine the root next.
			Package *root = rootPkg(pkg);
			if (pkg == root)
				return null;
			else
				return root;
		} else {
			// Default behaviour is to look at the nearest parent.
			return prev->parent();
		}
	}

	Named *ScopeLookup::find(Scope in, SimpleName *name) {
		// Regular path.
		for (NameLookup *at = in.top; at; at = nextCandidate(at)) {
			if (Named *found = storm::find(in, at, name))
				return found;
		}

		// Core.
		// TODO: We might not want to lookup inside 'core' always?
		if (Package *core = corePkg(in.top))
			if (Named *found = storm::find(in, core, name))
				return found;

		// We failed!
		return null;
	}

	Value ScopeLookup::value(Scope in, SimpleName *name, SrcPos pos) {
		// TODO: We may want to consider type aliases in the future, and implement 'void' that way.
		if (voidName != null && name->count() == 1) {
			SimplePart *last = name->last();
			if (*last->name == *voidName && last->params->empty())
				return Value();
		}

		try {
			Type *t = as<Type>(find(in, name));
			if (!t) {
				throw new (this) SyntaxError(pos, TO_S(engine(), name << S(" can not be resolved to a type.")));
			}

			return Value(t);
		} catch (CodeError *error) {
			if (error->pos.empty())
				error->pos = pos;
			throw;
		}
	}


	/**
	 * Scope.
	 */

	Scope::Scope() : top(null), lookup(null) {}

	Scope::Scope(NameLookup *top) : top(top) {
		assert(top);
		lookup = top->engine().scopeLookup();
	}

	Scope::Scope(NameLookup *top, ScopeLookup *lookup) : top(top), lookup(lookup) {}

	Scope::Scope(Scope parent, NameLookup *top) : top(top), lookup(parent.lookup) {}

	void Scope::deepCopy(CloneEnv *env) {
		// Should be OK to not do anything here... All our members are threaded objects.
	}

	Named *Scope::find(Name *name) const {
		try {
			SimpleName *simple = name->simplify(*this);
			if (simple)
				return find(simple);

		} catch (CodeError *error) {
			if (error->pos.empty()) {
				if (SrcName *s = as<SrcName>(name))
					error->pos = s->pos;
			}
			throw;
		}

		return null;
	}

	Named *Scope::find(SimpleName *name) const {
		if (lookup)
			return lookup->find(*this, name);

		return null;
	}

	Named *Scope::find(const wchar *name, Array<Value> *params) const {
		return find(new (params) SimpleName(new (params) Str(name), params));
	}

	Value Scope::value(Name *name, SrcPos pos) const {
		if (lookup) {
			SimpleName *simple = name->simplify(*this);
			if (simple)
				return lookup->value(*this, simple, pos);
		}

		throw new (name) SyntaxError(pos, TO_S(name, name << S(" can not be resolved to a type.")));
	}

	Value Scope::value(SrcName *name) const {
		return value(name, name->pos);
	}

	Scope Scope::withPos(SrcPos pos) const {
		if (!top)
			return *this;

		LookupPos *sub = new (top) LookupPos(pos);
		sub->parentLookup = top;
		return child(sub);
	}

	Scope rootScope(EnginePtr e) {
		return e.v.scope();
	}

	wostream &operator <<(wostream &to, const Scope &s) {
		if (!s.lookup) {
			to << L"<empty>";
		} else {
			to << L"<using ";
			to << s.lookup->type()->identifier();
			to << L"> ";

			bool first = true;
			for (NameLookup *at = s.top; at; at = ScopeLookup::nextCandidate(at)) {
				if (!first)
					to << " -> ";
				first = false;

				if (Named *n = as<Named>(at)) {
					to << n->identifier();
				} else if (LookupPos *p = as<LookupPos>(at)) {
					to << p->pos;
				} else {
					to << at->type()->identifier();
				}
			}
		}

		return to;
	}

	StrBuf &operator <<(StrBuf &to, Scope s) {
		return to << ::toS(s).c_str();
	}


	/**
	 * ScopeExtra.
	 */

	ScopeExtra::ScopeExtra() {
		search = new (this) Array<NameLookup *>();
		inSearch = new (this) Set<TObject *>();
	}

	ScopeExtra::ScopeExtra(Str *v) : ScopeLookup(v) {
		search = new (this) Array<NameLookup *>();
		inSearch = new (this) Set<TObject *>();
	}

	ScopeLookup *ScopeExtra::clone() const {
		ScopeExtra *copy = new (this) ScopeExtra();
		copy->search->append(search);
		copy->inSearch = new (this) Set<TObject *>(*inSearch);
		return copy;
	}

	void ScopeExtra::addExtra(NameLookup *lookup) {
		addExtra(lookup, true);
	}

	void ScopeExtra::addExtra(NameLookup *lookup, Bool useExports) {
		if (inSearch->has(lookup))
			return;

		inSearch->put(lookup);
		search->push(lookup);

		if (useExports) {
			if (Package *p = as<Package>(lookup)) {
				Array<Package *> *add = p->exports();
				for (Nat i = 0; i < add->count(); i++)
					addExtra(add->at(i), true);
			}
		}
	}

	Array<NameLookup *> *ScopeExtra::extra() const {
		return new (this) Array<NameLookup *>(*search);
	}

	Named *ScopeExtra::find(Scope in, SimpleName *name) {
		if (Named *found = ScopeLookup::find(in, name))
			return found;

		for (nat i = 0; i < search->count(); i++) {
			if (Named *found = storm::find(in, search->at(i), name))
				return found;
		}

		return null;
	}

}
