#include "stdafx.h"
#include "NameSet.h"
#include "Engine.h"
#include "Exception.h"
#include "Core/StrBuf.h"
#include "Core/Str.h"
#include <limits>

namespace storm {

	NameOverloads::NameOverloads() :
		items(new (this) Array<Named *>()),
		templates(new (this) Array<Template *>()) {}

	Bool NameOverloads::empty() const {
		return items->empty()
			&& templates->empty();
	}

	Nat NameOverloads::count() const {
		return items->count();
	}

	Named *NameOverloads::operator [](Nat id) const {
		return items->at(id);
	}

	Named *NameOverloads::at(Nat id) const {
		return items->at(id);
	}

	static bool equals(Array<Value> *a, Array<Value> *b) {
		if (a->count() != b->count())
			return false;

		for (Nat i = 0; i < a->count(); i++)
			if (a->at(i) != b->at(i))
				return false;

		return true;
	}

	void NameOverloads::add(Named *item) {
		for (Nat i = 0; i < items->count(); i++) {
			// If we try to re-insert the same object, we don't complain. This may happen when a
			// template "helps" the implementation to find a suitable overload (e.g. modifying a
			// parameter) which results in a duplicate. As we look for object identity here, we only
			// exclude cases where this is intentional (i.e. as it requires the template to remember
			// previously created objects).
			if (items->at(i) == item)
				return;

			if (storm::equals(item->params, items->at(i)->params)) {
				throw new (this) TypedefError(
					item->pos,
					TO_S(engine(), item << S(" is already defined as ") << items->at(i)->identifier()));
			}
		}

		items->push(item);
	}

	void NameOverloads::add(Template *item) {
		// There is no way to validate templates at this stage.
		templates->push(item);
	}

	Bool NameOverloads::remove(Named *item) {
		for (Nat i = 0; i < items->count(); i++) {
			if (items->at(i) == item) {
				items->remove(i);
				return true;
			}
		}

		return false;
	}

	Bool NameOverloads::remove(Template *item) {
		for (Nat i = 0; i < templates->count(); i++) {
			if (templates->at(i) == item) {
				templates->remove(i);
				return true;
			}
		}

		return false;
	}

	Named *NameOverloads::createTemplate(NameSet *owner, SimplePart *part, Scope source) {
		Named *found = null;
		for (Nat i = 0; i < templates->count(); i++) {
			Named *n = templates->at(i)->generate(part);
			if (found != null && n != null) {
				throw new (this) TypedefError(owner->pos, TO_S(engine(), S("Multiple template matches for: ") << part));
			} else if (n) {
				// Only pick it if it matches.
				if (part->matches(n, source) >= 0) {
					found = n;
				}
			}
		}

		if (found) {
			// TODO: Go through the regular 'add' of the NameSet!
			add(found);
			found->parentLookup = owner;
		}
		return found;
	}

	void NameOverloads::toS(StrBuf *to) const {
		for (Nat i = 0; i < items->count(); i++)
			*to << items->at(i) << L"\n";
		if (templates->any())
			*to << L"<" << templates->count() << L" templates>\n";
	}


	/**
	 * NameSet.
	 */

	NameSet::NameSet(Str *name) : Named(name) {
		init();
	}

	NameSet::NameSet(Str *name, Array<Value> *params) : Named(name, params) {
		init();
	}

	void NameSet::init() {
		loaded = false;
		loading = false;
		nextAnon = 0;

		if (engine().has(bootTemplates))
			lateInit();
	}

	void NameSet::lateInit() {
		Named::lateInit();

		if (!overloads)
			overloads = new (this) Map<Str *, NameOverloads *>();
	}

	void NameSet::compile() {
		forceLoad();

		for (Iter i = begin(), e = end(); i != e; ++i)
			i.v()->compile();
	}

	void NameSet::discardSource() {
		for (Iter i = begin(), e = end(); i != e; ++i)
			i.v()->discardSource();
	}

	void NameSet::watchAdd(Named *notifyTo) {
		if (!notify)
			notify = new (this) WeakSet<Named>();
		notify->put(notifyTo);
	}

	void NameSet::watchRemove(Named *notifyTo) {
		if (!notify)
			return;
		notify->remove(notifyTo);
	}

	void NameSet::notifyAdd(Named *what) {
		if (!notify)
			return;

		WeakSet<Named>::Iter i = notify->iter();
		while (Named *n = i.next()) {
			n->notifyAdded(this, what);
		}
	}

	void NameSet::notifyRemove(Named *what) {
		if (!notify)
			return;

		WeakSet<Named>::Iter i = notify->iter();
		while (Named *n = i.next()) {
			n->notifyRemoved(this, what);
		}
	}

	void NameSet::add(Named *item) {
		if (!overloads)
			overloads = new (this) Map<Str *, NameOverloads *>();

		overloads->at(item->name)->add(item);
		item->parentLookup = this;
		notifyAdd(item);
	}

	void NameSet::add(Template *item) {
		if (!overloads)
			overloads = new (this) Map<Str *, NameOverloads *>();

		overloads->at(item->name)->add(item);
	}

	void NameSet::remove(Named *item) {
		if (!overloads)
			return;

		NameOverloads *o = overloads->at(item->name);
		if (o->remove(item))
			notifyRemove(item);
		if (o->empty())
			overloads->remove(item->name);
	}

	void NameSet::remove(Template *item) {
		if (!overloads)
			return;

		NameOverloads *o = overloads->at(item->name);
		o->remove(item);
		if (o->empty())
			overloads->remove(item->name);
	}

	Str *NameSet::anonName() {
		StrBuf *buf = new (this) StrBuf();
		*buf << L"@ " << (nextAnon++);
		return buf->toS();
	}

	Array<Named *> *NameSet::content() {
		Array<Named *> *r = new (this) Array<Named *>();
		for (Overloads::Iter at = overloads->begin(); at != overloads->end(); ++at) {
			NameOverloads &o = *at.v();
			for (Nat i = 0; i < o.count(); i++)
				r->push(o[i]);
		}
		return r;
	}

	void NameSet::forceLoad() {
		if (loaded)
			return;

		if (loading) {
			// This happens quite a lot...
			// WARNING(L"Recursive loading attempted for " << name);
			return;
		}

		loading = true;
		try {
			if (loadAll())
				loaded = true;
		} catch (...) {
			loading = false;
			throw;
		}
		loading = false;
	}

	Named *NameSet::find(SimplePart *part, Scope source) {
		if (Named *found = tryFind(part, source))
			return found;

		if (loaded)
			return null;

		if (!loadName(part))
			forceLoad();

		return tryFind(part, source);
	}

	Named *NameSet::tryFind(SimplePart *part, Scope source) {
		if (!overloads)
			return null;

		Overloads::Iter i = overloads->find(part->name);
		if (i == overloads->end())
			return null;

		return tryFind(part, source, i.v());
	}

	Named *NameSet::tryFind(SimplePart *part, Scope source, NameOverloads *from) {
		// Note: We do this in two steps. First, we find the best match, and keep track of whether
		// or not there are multiple instances of that or not. If there are multiple instances of
		// the best match, we need to produce an error. To do that, we loop through the candidates a
		// second time (since the error path is generally not critical).
		Named *bestCandidate = null;
		Bool multipleBest = false;
		Int best = std::numeric_limits<Int>::max();

		for (Nat i = 0; i < from->count(); i++) {
			Named *candidate = from->at(i);

			// Ignore ones that are not visible. Note: We delegate this to the part, so that it may
			// modify the default behavior.
			if (!part->visible(candidate, source))
				continue;

			Int badness = part->matches(candidate, source);
			if (badness < 0 || badness > best)
				continue;

			if (badness == best) {
				// Multiple best matches so far. We can't keep track of them without allocating memory.
				multipleBest = true;
			} else {
				multipleBest = false;
				best = badness;
				bestCandidate = candidate;
			}
		}

		// If we have a badness above zero, we might need to create a template to get a better
		// match.
		if (best > 0) {
			if (Named *created = from->createTemplate(this, part, source)) {
				if (created && part->visible(created, source)) {
					// Check suitability of the newly created template. Note: We more or less expect
					// that the created template is a perfect match. If we get a higher badness,
					// something is probably wrong with the implementation of the template.
					Int badness = part->matches(created, source);
					if (badness >= 0 && badness < best) {
						bestCandidate = created;
						best = badness;
						multipleBest = false;
					}
				}
			}
		}

		if (!multipleBest) {
			// We have an answer!
			return bestCandidate;
		}

		// Error case, we need to find all candidates.
		StrBuf *msg = new (this) StrBuf();
		*msg << S("Multiple possible matches for ") << this << S(", all with badness ") << best << S("\n");

		for (Nat i = 0; i < from->count(); i++) {
			Named *candidate = from->at(i);

			if (!part->visible(candidate, source))
				continue;

			Int badness = part->matches(candidate, source);
			if (badness == best)
				*msg << S("  Could be: ") << candidate->identifier() << S("\n");
		}

		throw new (this) LookupError(msg->toS());
	}

	Bool NameSet::loadName(SimplePart *part) {
		// Default implementation if the derived class does not support lazy-loading.
		// Report some matches may be found using 'loadAll'.
		return false;
	}

	Bool NameSet::loadAll() {
		// Default implementation if the derived class does not support lazy-loading.
		// Report done.
		return true;
	}

	void NameSet::toS(StrBuf *to) const {
		for (Overloads::Iter i = overloads->begin(); i != overloads->end(); ++i) {
			*to << i.v();
		}
	}

	NameSet::Iter::Iter() : name(), pos(0) {}

	NameSet::Iter::Iter(Map<Str *, NameOverloads *> *c) : name(c->begin()), pos(0) {
		advance();
	}

	Bool NameSet::Iter::operator ==(const Iter &o) const {
		// Either both at end or none.
		if (name == MapIter())
			return o.name == MapIter();

		if (name != o.name)
			return false;

		return pos == o.pos;
	}

	Bool NameSet::Iter::operator !=(const Iter &o) const {
		return !(*this == o);
	}

	Named *NameSet::Iter::v() const {
		return name.v()->at(pos);
	}

	void NameSet::Iter::advance() {
		while (name != MapIter() && pos >= name.v()->count()) {
			++name;
			pos = 0;
		}
	}

	NameSet::Iter &NameSet::Iter::operator ++() {
		pos++;
		advance();
		return *this;
	}

	NameSet::Iter NameSet::Iter::operator ++(int) {
		Iter i(*this);
		++*this;
		return i;
	}

	NameSet::Iter NameSet::begin() const {
		Iter r;
		if (overloads)
			r = Iter(overloads);
		return r;
	}

	NameSet::Iter NameSet::end() const {
		return Iter();
	}

	Array<Named *> *NameSet::findName(Str *name) const {
		Array<Named *> *result = new (this) Array<Named *>();

		if (NameOverloads *f = overloads->get(name, null)) {
			result->reserve(f->count());
			for (Nat i = 0; i < f->count(); i++)
				result->push(f->at(i));
		}

		return result;
	}

	void NameSet::dbg_dump() const {
		PLN(L"Name set:");
		for (Overloads::Iter i = overloads->begin(); i != overloads->end(); ++i) {
			PLN(L" " << i.k() << L":");
			NameOverloads *o = i.v();
			for (Nat i = 0; i < o->count(); i++) {
				PLN(L"  " << o->at(i)->identifier());
			}
		}
	}

}
