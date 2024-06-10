#include "stdafx.h"
#include "Table.h"
#include "Exception.h"

namespace storm {
	namespace syntax {
		namespace glr {

			/**
			 * Action.
			 */

			Action::Action(Regex regex, Nat action) : regex(regex), action(action) {}

			void Action::toS(StrBuf *to) const {
				*to << S("\"") << regex << S("\" -> ") << action;
			}

			/**
			 * State.
			 */

			State::State(ItemSet items) : items(items) {}

			void State::toS(StrBuf *to) const {
				toS(to, null);
			}

			void State::toS(StrBuf *to, Syntax *syntax) const {
				if (actions && rules) {
					*to << L"Actions:\n";
					Indent z(to);
					for (Nat i = 0; i < actions->count(); i++)
						*to << L"shift " << actions->at(i) << L"\n";

					for (Map<Nat, Nat>::Iter i = rules->begin(), end = rules->end(); i != end; ++i)
						*to << L"goto " << syntax->ruleName(i.k()) << L" -> " << i.v() << L"\n";

					for (Nat i = 0; i < reduce->count(); i++)
						*to << L"reduce " << Item(syntax, reduce->at(i)).toS(syntax) << L"\n";

					for (Nat i = 0; i < reduceOnEmpty->count(); i++) {
						Action a = reduceOnEmpty->at(i);
						*to << L"reduce " << Item(syntax, a.action).toS(syntax)
							<< L" when \"" << a.regex << L"\" is empty\n";
					}

				} else {
					*to << L"<to be computed>\n";
				}

				*to << L"Items:\n";
				for (Nat i = 0; i < items.count(); i++) {
					if (syntax)
						*to << items[i].toS(syntax);
					else
						*to << items[i];
					*to << L"\n";
				}
			}


			/**
			 * Full table.
			 */

			Table::Table(Syntax *syntax) : syntax(syntax) {
				lookup = new (this) Map<ItemSet, Nat>();
				states = new (this) Array<State *>();
			}

			Bool Table::empty() const {
				return lookup->empty()
					&& states->empty();
			}

			Nat Table::state(ItemSet s) {
				Map<ItemSet, Nat>::Iter found = lookup->find(s);
				if (found != lookup->end())
					return found.v();

				Nat id = states->count();
				lookup->put(s, id);
				states->push(new (this) State(s));
				return id;
			}

			State *Table::state(Nat id) {
				State *s = states->at(id);
				if (!s->actions)
					fill(s);
				return s;
			}

			void Table::populate() {
				for (Nat i = 0; i < states->count(); i++) {
					state(i);
				}
			}

			void Table::fill(State *state) {
				state->actions = new (this) Array<Action>();
				state->rules = new (this) Map<Nat, Nat>();
				state->reduce = new (this) Array<Nat>();
				state->reduceOnEmpty = new (this) Array<Action>();

				if (state->items.count() > 10)
					fillLarge(state);
				else
					fillSmall(state);
			}

			void Table::fillLarge(State *state) {
				// De-duplicate reduction items.
				Set<Nat> *reduce = new (this) Set<Nat>();

				// Keep track of distinct shift- and reduce actions.
				Map<Nat, ItemSet> *rule = new (this) Map<Nat, ItemSet>();
				Map<Regex, ItemSet> *shift = new (this) Map<Regex, ItemSet>();

				ItemSet items = state->items;
				for (Nat i = 0; i < items.count(); i++) {
					Item item = items[i];
					if (item.end()) {
						// Insert a reduce action.
						if (!reduce->has(item.id)) {
							reduce->put(item.id);
							state->reduce->push(item.id);
						}
					} else if (item.isRule(syntax)) {
						// Prepare adding a reduce action.
						rule->at(item.nextRule(syntax)).push(engine(), item.next(syntax));
					} else {
						// Prepare adding a shift action.
						shift->at(item.nextRegex(syntax)).push(engine(), item.next(syntax));
					}
				}

				for (Map<Nat, ItemSet>::Iter i = rule->begin(), end = rule->end(); i != end; ++i) {
					state->rules->put(i.k(), this->state(i.v().expand(syntax)));
				}

				for (Map<Regex, ItemSet>::Iter i = shift->begin(), end = shift->end(); i != end; ++i) {
					Action shift(i.k(), this->state(i.v().expand(syntax)));
					state->actions->push(shift);

					if (shift.regex.matchesEmpty()) {
						// We need to pretend a production can be reduced here!
						Nat rule = Syntax::prodESkip | shift.action;
						state->rules->put(rule, shift.action);

						// We need to add the reduction as well.
						state->reduceOnEmpty->push(Action(shift.regex, rule));
					}
				}
			}

			void Table::fillSmall(State *state) {
				// De-duplicate reduction items.
				Set<Nat> *reduce = new (this) Set<Nat>();

				ItemSet items = state->items;
				Array<Bool> *used = new (this) Array<Bool>(items.count(), false);
				for (Nat i = 0; i < items.count(); i++) {
					if (used->at(i))
						continue;
					used->at(i) = true;

					Item item = items[i];
					if (item.end()) {
						// Insert a reduce action.
						if (!reduce->has(item.id)) {
							reduce->put(item.id);
							state->reduce->push(item.id);
						}
					} else if (item.isRule(syntax)) {
						// Add new states to the goto-table.
						Nat rule = item.nextRule(syntax);
						state->rules->put(rule, createGoto(i, items, used, rule));
					} else {
						// Add a shift action.
						Action shift = createShift(i, items, used, item.nextRegex(syntax));
						state->actions->push(shift);

						if (shift.regex.matchesEmpty()) {
							// We need to pretend a production can be reduced here!
							Nat rule = Syntax::prodESkip | shift.action;
							state->rules->put(rule, shift.action);

							// We need to add the reduction as well.
							state->reduceOnEmpty->push(Action(shift.regex, rule));
						}
					}
				}
			}

			Nat Table::createGoto(Nat start, ItemSet items, Array<Bool> *used, Nat rule) {
				ItemSet result;
				result.push(engine(), items[start].next(syntax));

				for (Nat i = start + 1; i < items.count(); i++) {
					if (used->at(i))
						continue;

					Item item = items[i];
					if (item.end() || !item.isRule(syntax))
						continue;

					if (item.nextRule(syntax) != rule)
						continue;

					used->at(i) = true;
					result.push(engine(), item.next(syntax));
				}

				return state(result.expand(syntax));
			}

			Action Table::createShift(Nat start, ItemSet items, Array<Bool> *used, Regex regex) {
				ItemSet result;
				result.push(engine(), items[start].next(syntax));

				for (Nat i = start + 1; i < items.count(); i++) {
					if (used->at(i))
						continue;

					Item item = items[i];
					if (item.end() || item.isRule(syntax))
						continue;

					Regex r = item.nextRegex(syntax);
					if (r != regex)
						continue;

					used->at(i) = true;
					result.push(engine(), item.next(syntax));
				}

				return Action(regex, state(result.expand(syntax)));
			}

			void Table::toS(StrBuf *to) const {
				*to << L"Parse table, " << states->count() << L" states.\n";
				for (Nat i = 0; i < states->count(); i++) {
					*to << L"State " << i << L":\n";
					Indent z(to);

					states->at(i)->toS(to, syntax);
					*to << L"\n";
				}
			}

		}
	}
}
