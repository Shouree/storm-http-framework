#include "stdafx.h"
#include "State.h"
#include "Parser.h"

namespace storm {
	namespace syntax {
		namespace earley {

			StatePtr::StatePtr() : step(-1), index(-1) {}

			StatePtr::StatePtr(Nat step, Nat index) : step(step), index(index) {}

			StrBuf &operator <<(StrBuf &to, StatePtr p) {
				return to << L"(" << p.step << L", " << p.index << L")";
			}

			wostream &operator <<(wostream &to, StatePtr p) {
				return to << L"(" << p.step << L", " << p.index << L")";
			}

			State::State() : from(0) {}

			State::State(ProductionIter pos, Nat from) : pos(pos), from(from) {}

			State::State(ProductionIter pos, Nat from, StatePtr prev)
				: pos(pos), from(from), prev(prev) {}

			State::State(ProductionIter pos, Nat from, StatePtr prev, StatePtr completed)
				: pos(pos), from(from), prev(prev), completed(completed) {}

			StrBuf &operator <<(StrBuf &to, State s) {
				return to << L"{" << s.pos << L", " << s.from << L", " << s.prev << L", " << s.completed << L"}";
			}

			wostream &operator <<(wostream &to, State s) {
				return to << L"{" << s.pos << L", " << s.from << L", " << s.prev << L", " << s.completed << L"}";
			}

			StateSet::StateSet() : chunks(null) {
				stateArrayType = StormInfo<State>::handle(engine()).gcArrayType;
			}

			void StateSet::push(Parser *parser, const State &s) {
				if (!s.valid())
					return;

				// TODO: Optimize this somehow. A lot of time is spent here!
				Nat chunkCount = chunks ? Nat(chunks->filled) : 0;
				for (nat c = 0; c < chunkCount; c++) {
					GcArray<State> *chunk = chunks->v[c];
					Nat cnt = Nat(chunk->filled);
					for (nat i = 0; i < cnt; i++) {
						State &old = chunk->v[i];
						if (old == s) {
							// Generated this state alreadly, shall we update it?

							// Keep the largest in a lexiographic ordering.
							if (execOrder(parser, s, old) != before)
								return;

							chunk->v[i] = s;
							return;
						}
					}
				}

				// Push the new state somewhere.
				GcArray<State> *last = null;
				if (chunks == null) {
					ensure(1);
					last = chunks->v[0];
				} else {
					last = chunks->v[chunks->filled - 1];
				}

				if (last->filled >= chunkSize) {
					ensure(Nat(chunks->filled + 1));
					last = chunks->v[chunks->filled - 1];
				}

				last->v[last->filled++] = s;
			}

			void StateSet::ensure(Nat n) {
				// Need to resize the array?
				if (chunks == null || chunks->count < n) {
					Nat newCount = 1;
					if (chunks)
						newCount = 2 * Nat(chunks->count);
					newCount = max(n, newCount);

					GcArray<GcArray<State> *> *old = chunks;
					chunks = runtime::allocArray<GcArray<State> *>(engine(), &pointerArrayType, newCount);
					if (old) {
						memcpy(chunks->v, old->v, sizeof(void *)*old->count);
						chunks->filled = old->filled;
					}
				}

				// Make sure all entries are filled.
				while (chunks->filled < n) {
					chunks->v[chunks->filled++] = runtime::allocArray<State>(engine(), stateArrayType, chunkSize);
				}
			}

			void StateSet::push(Parser *parser, ProductionIter pos, Nat from) {
				push(parser, pos, from, StatePtr(), StatePtr());
			}

			void StateSet::push(Parser *parser, ProductionIter pos, Nat from, StatePtr prev) {
				push(parser, pos, from, prev, StatePtr());
			}

			void StateSet::push(Parser *parser, ProductionIter pos, Nat from, StatePtr prev, StatePtr completed) {
				if (!pos.valid())
					return;

				push(parser, State(pos, from, prev, completed));
			}

			StateSet::Order StateSet::execOrder(Parser *parser, const StatePtr &aPtr, const StatePtr &bPtr) const {
				// Invalid states have no ordering.
				if (aPtr == StatePtr() || bPtr == StatePtr())
					return none;

				// Same state, realize that quickly!
				if (aPtr == bPtr)
					return none;

				return execOrder(parser, parser->state(aPtr), parser->state(bPtr));
			}

			StateSet::Order StateSet::execOrder(Parser *parser, const State &a, const State &b) const {
				// Check which has the highest priority.
				if (a.priority() != b.priority())
					return (a.priority() > b.priority()) ? before : after;

				// If they are different productions and noone has a higher priority than the other, the
				// ordering is undefined.
				if (a.production() != b.production())
					return none;

				// Order them lexiographically to see which has the highest priority!
				StateArray aStates(engine());
				prevCompleted(parser, a, aStates);
				StateArray bStates(engine());
				prevCompleted(parser, b, bStates);

				nat to = min(aStates.count(), bStates.count());
				for (nat i = 0; i < to; i++) {
					Order order = execOrder(parser, aStates[i], bStates[i]);
					if (order != none)
						return order;
				}

				// Pick the longer one in case they are equal so far. This makes * and + greedy.
				if (aStates.count() > bStates.count()) {
					return before;
				} else if (aStates.count() < bStates.count()) {
					return after;
				}

				// The rules are equal as far as we are concerned.
				return none;
			}

			void StateSet::prevCompleted(Parser *parser, const State &from, StateArray &to) const {
				to.clear();

				// Note: the first state is never completed, so we skip that.
				for (const State *at = &from; at->prev != StatePtr(); at = &parser->state(at->prev)) {
					to.push(at->completed);
				}

				to.reverse();
			}

		}
	}
}
