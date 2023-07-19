#pragma once
#include "Core/Object.h"
#include "Core/GcArray.h"
#include "Core/Array.h"
#include "Compiler/Thread.h"
#include "Compiler/Syntax/InfoErrors.h"
#include "Syntax.h"
#include "Tree.h"
#include "ParentReq.h"

namespace storm {
	namespace syntax {
		namespace glr {
			STORM_PKG(lang.bnf.glr);

			/**
			 * Stack items in the GLR parser.
			 *
			 * These closely resemble what the paper describes as 'links'. One node consists of one
			 * instance of this class. All instances also describe a 'link', and multiple instances
			 * can be chained together to represent all links.
			 *
			 * TODO: It could be useful to merge these into larger chunks and use one-dimensional
			 * ID:s for pointers to them, like what we do with the tree representation.
			 */
			class StackItem : public Object {
				STORM_CLASS;
			public:
				// Create.
				STORM_CTOR StackItem();
				STORM_CTOR StackItem(Nat state, Nat pos);
				STORM_CTOR StackItem(Nat state, Nat pos, StackItem *prev, Nat tree);
				STORM_CTOR StackItem(Nat state, Nat pos, StackItem *prev, Nat tree, ParentReq req);

				// State at this point in the stack.
				Nat state;

				// Position in the input. TODO: Can we remove this somehow?
				Nat pos;

				// Part of the syntax tree for this node.
				Nat tree;

				// Previous item in the stack.
				MAYBE(StackItem *) prev;

				// More previous states? Forms a linked list of multiple StackItem nodes at the same
				// level (we ignore 'state' there) of more previous items.
				MAYBE(StackItem *) morePrev;

				// Required parent productions for this state.
				ParentReq required;

				// Insert a node in the 'morePrev' chain if it is not already there. Returns 'true' if inserted.
				Bool STORM_FN insert(TreeStore *store, StackItem *item);
				Bool insert(TreeStore *store, StackItem *item, Bool &usedTree);

				// Equality check and hashing.
				virtual Bool STORM_FN operator ==(const StackItem &o) const;
				virtual Nat STORM_FN hash() const;

				// Output.
				virtual void STORM_FN toS(StrBuf *to) const;

			private:
				// Update 'tree' in this object if the provided tree has a higher priority than this.
				Bool updateTree(TreeStore *store, Nat newTree);
			};


			/**
			 * Set of future stack sets. Implements a sequence of sets where lookup is constant time
			 * and popping the front is also constant time. Any access extends the structure to the
			 * desired length.
			 */
			class FutureStacks : public ObjectOn<Compiler> {
				STORM_CLASS;
			public:
				STORM_CTOR FutureStacks();

				// Get the topmost set.
				MAYBE(Array<StackItem *> *) STORM_FN top();

				// Pop the topmost set, shifting all other indices one step.
				void STORM_FN pop();

				// Insert an item at location 'pos', without attempting to merge nodes if the state
				// is already present. Returns eithere 'item' on success, or the item that is
				// already present in the set at the indicated position.
				StackItem *STORM_FN putRaw(Nat pos, StackItem *item);

				// Insert an item at location 'pos'. The top is at location 0. Returns false if
				// rejected due to a duplicate node.
				Bool STORM_FN put(Nat pos, TreeStore *store, StackItem *item);

				// Set the top item to some value.
				void STORM_FN set(Nat pos, Array<StackItem *> *v);

			private:
				// Storage. Used as a circular queue. Size is always a power of two.
				GcArray<Array<StackItem *> *> *data;

				// First element position.
				Nat first;

				// Grow to fit at least 'n' elements.
				void grow(Nat n);

				// Number of elements in 'data' currently.
				inline Nat count() const { return data ? Nat(data->count) : 0; }

				// Wrap 'n' to the size of data.
				inline Nat wrap(Nat n) const { return data ? (n & (data->count - 1)) : 0; }
			};

		}
	}
}
