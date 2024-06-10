#pragma once
#include "Syntax.h"
#include "Item.h"
#include "Core/Map.h"
#include "Core/Array.h"

namespace storm {
	namespace syntax {
		namespace glr {
			STORM_PKG(lang.bnf.glr);

			/**
			 * One shift or reduce entry in the action table.
			 */
			class Action {
				STORM_VALUE;
			public:
				// Create.
				STORM_CTOR Action(Regex regex, Nat action);

				// Regular expression to match.
				Regex regex;

				// Go to this state, or reduce this production.
				Nat action;

				// Output.
				void STORM_FN toS(StrBuf *to) const;
			};


			/**
			 * One row in the LR table.
			 */
			class State : public ObjectOn<Compiler> {
				STORM_CLASS;
			public:
				// Create.
				STORM_CTOR State(ItemSet items);

				// The item set of this state.
				ItemSet items;

				// All shift actions in here. If the array is 'null', the actions need to be created.
				MAYBE(Array<Action> *) actions;

				// The goto table. If 'null' it needs to be created. (Note: can not be named goto...)
				MAYBE(Map<Nat, Nat> *) rules;

				// If we're LR(0), reduce these states. This is required in more powerful formalsism
				// as well since we want to be able to reduce the start production even if the
				// lookahead is wrong.
				MAYBE(Array<Nat> *) reduce;

				// Reduce these productions when the lookahead matches zero characters (regexes are
				// greedy, so some regexes do not match zero characters at all positions even though
				// they can do that).
				MAYBE(Array<Action> *) reduceOnEmpty;

				// To string.
				virtual void STORM_FN toS(StrBuf *to) const;
				void STORM_FN toS(StrBuf *to, Syntax *syntax) const;
			};


			/**
			 * Lazily generated parse table (currently LR(0)).
			 *
			 * Basically associates item sets to indices and generates new indices on demand.
			 */
			class Table : public ObjectOn<Compiler> {
				STORM_CLASS;
			public:
				// Create.
				STORM_CTOR Table(Syntax *syntax);

				// Is the table empty?
				Bool STORM_FN empty() const;

				// Find the index of a state set.
				Nat STORM_FN state(ItemSet s);

				// Get the actual state from an index.
				State *STORM_FN state(Nat id);

				// Eagerly compute all states in the table.
				void STORM_FN populate();

				// To string.
				virtual void STORM_FN toS(StrBuf *to) const;

			private:
				// All syntax.
				Syntax *syntax;

				// Lookup of item-sets to state id.
				Map<ItemSet, Nat> *lookup;

				// States.
				Array<State *> *states;

				// Compute the contents of a state.
				void fill(State *state);

				// Bucketing approach for large and small states.
				void fillLarge(State *state);
				void fillSmall(State *state);

				// Create the actions for a state. Used in 'findSmall'.
				Action createShift(Nat start, ItemSet items, Array<Bool> *used, Regex regex);
				Nat createGoto(Nat start, ItemSet items, Array<Bool> *used, Nat rule);
			};

		}
	}
}
