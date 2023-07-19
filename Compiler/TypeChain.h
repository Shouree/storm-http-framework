#pragma once
#include "Thread.h"
#include "Core/GcArray.h"
#include "Core/Array.h"
#include "Core/WeakSet.h"

namespace storm {
	STORM_PKG(core.lang);

	/**
	 * Implements a representation of the type-hierarchy in Storm where it is O(1) to check if a
	 * class is a subclass of another. Using Cohen's algorithm.
	 *
	 * TODO: Realize when a type died and remove it from any other TypeChain:s then!
	 */
	class TypeChain : public ObjectOn<Compiler> {
		STORM_CLASS;
	public:
		// Create a new chain owned by a type.
		STORM_CTOR TypeChain(Type *owner);

		// Set the super type to 'o'.
		void STORM_FN super(MAYBE(TypeChain *) o);
		void STORM_FN super(MAYBE(Type *) o);

		// Get the super type.
		Type *STORM_FN super() const;

		// Is this type derived from 'o'?
		Bool STORM_FN isA(const TypeChain *o) const;
		Bool STORM_FN isA(const Type *o) const;

		// Get the distance from another type, or -1 if not related.
		Int STORM_FN distance(const TypeChain *o) const;
		Int STORM_FN distance(const Type *o) const;

		// Iterator for children.
		class Iter {
			STORM_VALUE;
			friend class TypeChain;
		public:
			// Get the next element.
			MAYBE(Type *) STORM_FN next();

		private:
			// Create.
			Iter(WeakSet<TypeChain> *src);

			// Create null iterator.
			Iter();

			// Original iterator. (typedef since the preprocessor fails otherwise).
			typedef WeakSet<TypeChain>::Iter It;
			UNKNOWN(WeakSetBase::Iter) It src;
		};

		// Get all currently known direct children. The result is _not_ ordered in any way.
		Iter STORM_FN children() const;

		// Late initialization. Requires templates.
		void lateInit();

	private:
		// Our owner.
		Type *owner;

		// Chain of super types (eg. our supertype, that supertype, and so on).
		// If we have no parents, 'chain' is set to null.
		GcArray<TypeChain *> *chain;

		// Children.
		WeakSet<TypeChain> *child;

		// Update our type chain.
		void updateSuper(const TypeChain *from);

		// Notify children about changes.
		void notify() const;

		// Clear our chain.
		void clearSuper();

		// Get the super TypeChain object.
		TypeChain *superChain() const;

		// Size of the chain.
		inline Nat chainCount() const { return chain ? Nat(chain->count) : 1; }

		// Element 'n' in the chain.
		inline TypeChain *chainAt(Nat n) const { return chain ? chain->v[n] : const_cast<TypeChain *>(this); }
	};

}
