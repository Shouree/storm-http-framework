#include "stdafx.h"
#include "TemplateList.h"
#include "Engine.h"
#include "Type.h"
#include "Exception.h"

namespace storm {

	/**
	 * Compatible with GcArray. We're also storing a pointer to another node in here, so 'elements'
	 * will always be one too large. Therefore, we store the 'correct' count inside 'count'.
	 */
	struct TemplateList::Node {
		size_t elements;
		size_t count;

		// If we're done.
		Type *done;

		// Instantiations here. The index 0 is used if for the parameter 'null'.
		Node *next[1];
	};

	TemplateList::TemplateList(World *world, TemplateCppFn *t) : templ(t), world(world), addedTo(null) {
		lock = new (this) Lock();
	}

	void TemplateList::addTo(NameSet *to) {
		if (!addedTo) {
			addedTo = to;
			if (!templ->placeholder())
				to->add(templ);
			addTo(root, to);
		}
	}

	void TemplateList::addTo(Node *at, NameSet *to) {
		if (!at)
			return;

		try {
			if (at->done)
				to->add(at->done);
		} catch (const TypedefError *e) {
			// Sometimes we have a different notion of what is acceptable.
			WARNING(e->toS());
		}

		for (nat i = 0; i < at->count; i++)
			addTo(at->next[i], to);
	}

	Type *TemplateList::find(Nat *elems, Nat count) {
		Lock::Guard z(lock);

		Type *found = findAt(elems, count, root, 0);
		if (!found) {
			found = generate(elems, count);
			insertAt(elems, count, found, root, 0);
		}

		return found;
	}

	Type *TemplateList::findAt(Nat *elems, Nat count, Node *node, Nat at) {
		if (!node)
			return null;

		if (count == at)
			return node->done;

		Nat now = elems[at];
		if (now == Nat(-1))
			now = 0;
		else
			now++;

		if (node->count <= now)
			return null;

		return findAt(elems, count, node->next[now], at + 1);
	}

	void TemplateList::insertAt(Nat *elems, Nat count, Type *insert, Node *&node, Nat at) {
		if (count == at) {
			// Insert it here!
			if (!node)
				node = allocNode(0);

			node->done = insert;
			return;
		}

		Nat now = elems[at];
		if (now == Nat(-1))
			now = 0;
		else
			now++;

		// Create a new node?
		if (!node)
			node = allocNode(now + 1);

		// Increase size of existing node?
		if (node->count <= now)
			node = allocNode(node, now + 1);

		insertAt(elems, count, insert, node->next[now], at + 1);
	}

	void TemplateList::forNamed(NamedFn fn) {
		forNamed(root, fn);
	}

	void TemplateList::forNamed(Node *node, NamedFn fn) {
		if (!node)
			return;

		if (node->done)
			(*fn)(node->done);

		for (nat i = 0; i < node->count; i++)
			forNamed(node->next[i], fn);
	}

	TemplateList::Node *TemplateList::allocNode(Nat count) {
		Node *n = (Node *)engine().gc.allocArray(&pointerArrayType, count + 1);
		n->count = count;
		return n;
	}

	TemplateList::Node *TemplateList::allocNode(const Node *original, Nat count) {
		Node *mem = allocNode(count);
		mem->done = original->done;
		for (nat i = 0; i < min(count, nat(original->count)); i++)
			mem->next[i] = original->next[i];
		return mem;
	}

	Type *TemplateList::generate(Nat *elems, Nat count) {
		Engine &e = engine();
		ValueArray *types = new (e) ValueArray();
		types->reserve(count);

		for (nat i = 0; i < count; i++) {
			if (elems[i] == Nat(-1)) {
				types->push(Value());
			} else {
				Type *t = world->types[elems[i]];
				assert(t, L"Type with id " + ::toS(elems[i]) + L" not found.");
				types->push(Value(t));
			}
		}

		if (addedTo && e.has(bootTemplates)) {
			// Was the type already added from somewhere else?
			SimplePart *p = new (e) SimplePart(templ->name, types->toArray());
			if (Type *t = as<Type>(addedTo->find(p, Scope())))
				return t;
		}

		Type *r = templ->generate(types);
		assert(r, L"Invalid template usage for types " + ::toS(types));
		if (addedTo) {
			try {
				addedTo->add(r);
			} catch (const TypedefError *e) {
				// Sometimes, we have a different notion of what is acceptable together.
				WARNING(e->toS());
			}
		}
		return r;
	}

}
