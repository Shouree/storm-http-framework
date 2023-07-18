#pragma once
#include "CppName.h"
#include "SrcPos.h"
#include "Auto.h"
#include "Exception.h"

/**
 * A map from C++ names into some type. Acts like both a searchable map as well as an ordered array.
 *
 * Assumes T::name, T::pos and T::id exists. Auto<T> has to be usable.
 */
template <class T>
class NameMap {
public:
	NameMap(const vector<CppName> &usingExpr, const map<CppName, CppName> *aliases = null) :
		usingExpr(usingExpr), aliases(aliases) {}

	// Insert a new element at the end.
	void insert(const Auto<T> &v) {
		if (lookup.count(v->name))
			throw Error(L"The name " + toS(v->name) + L" is defined twice!", v->pos);

		lookup.insert(make_pair(v->name, nat(order.size())));
		order.push_back(v);
	}

	// Order the types according to a predicate.
	template <class Pred>
	void sort(const Pred &pred) {
		std::sort(order.begin(), order.end(), pred);

		for (nat i = 0; i < order.size(); i++) {
			order[i]->id = i;
			lookup[order[i]->name] = i;
		}
	}

	// Total number of elements.
	nat size() const {
		return nat(order.size());
	}

	// Access elements.
	const Auto<T> &operator[] (nat id) const {
		return order[id];
	}

	// Find by name.
	T *findUnsafe(const CppName &name, CppName context) const {
		map<CppName, nat>::const_iterator i = lookup.find(name);
		if (i != lookup.end())
			return order[i->second].borrow();

		while (!context.empty()) {
			CppName candidate = context + name;

			if (aliases) {
				map<CppName, CppName>::const_iterator j = aliases->find(candidate);
				if (j != aliases->end()) {
					i = lookup.find(j->second);
					if (i == lookup.end())
						throw Error(L"The target " + j->second + L" for alias " + j->first + L" was not found. "
									L"Note that aliases must specify the full name of the type!", SrcPos());
					return order[i->second].borrow();
				}
			}

			i = lookup.find(candidate);
			if (i != lookup.end())
				return order[i->second].borrow();

			context = context.parent();
		}

		// Reachable via 'using namespace'?
		for (nat j = 0; j < usingExpr.size(); j++) {
			i = lookup.find(usingExpr[j] + name);
			if (i != lookup.end())
				return order[i->second].borrow();
		}

		return null;
	}

	// Find by name, throws exception if not found.
	T *find(const CppName &name, CppName context, const SrcPos &pos) const {
		T *t = findUnsafe(name, context);
		if (!t)
			throw Error(L"The name " + name + L" is not known to Storm.", pos);
		return t;
	}


private:
	// All using-expressions.
	const vector<CppName> &usingExpr;

	// Type aliases.
	const map<CppName, CppName> *aliases;

	// Quick lookups.
	map<CppName, nat> lookup;

	// Order.
	vector<Auto<T>> order;
};

