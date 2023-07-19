#pragma once
#include "Compiler/Syntax/Production.h"
#include "Core/PODArray.h"

namespace storm {
	namespace syntax {
		namespace earley {
			STORM_PKG(lang.bnf.earley);

			class Parser;

			/**
			 * Pointer to a state in some StateSet. Represented by two integers: the step and the index
			 * into that step. Null is represented by step and index being (Nat)-1.
			 */
			class StatePtr {
				STORM_VALUE;
			public:
				// Create invalid state pointer.
				STORM_CTOR StatePtr();

				// Create to a specific state.
				STORM_CTOR StatePtr(Nat step, Nat index);

				// The step.
				Nat step;

				// Index inside that step.
				Nat index;

				// Compare.
				inline Bool STORM_FN operator ==(const StatePtr &o) const {
					return step == o.step && index == o.index;
				}

				inline Bool STORM_FN operator !=(const StatePtr &o) const {
					return !(*this == o);
				}
			};

			StrBuf &STORM_FN operator <<(StrBuf &to, StatePtr p);
			wostream &operator <<(wostream &to, StatePtr p);

			/**
			 * State used during parsing (in Parser.h/cpp). Contains a location into a production, a
			 * step where the option was instantiated, a pointer to the previous step and optionally the
			 * step that was completed.
			 */
			class State {
				STORM_VALUE;
			public:
				// Create.
				STORM_CTOR State();
				STORM_CTOR State(ProductionIter pos, Nat from);
				STORM_CTOR State(ProductionIter pos, Nat from, StatePtr prev);
				STORM_CTOR State(ProductionIter pos, Nat from, StatePtr prev, StatePtr completed);

				// Position in the production.
				ProductionIter pos;

				// The step where this production was instantiated.
				Nat from;

				// Previous instance of this state.
				StatePtr prev;

				// State that completed this state. If set, this means that pos->token is a rule token.
				StatePtr completed;

				// Is this a valid state (ie. does it have a valid position?)
				inline Bool valid() const {
					return pos.valid();
				}

				// Get the priority for the rule associated with this state.
				inline Int priority() const {
					return pos.production()->priority;
				}

				// Get the production this state represents.
				inline Production *production() const {
					return pos.production();
				}

				// See if this state is a Rule token.
				inline RuleToken *getRule() const {
					Token *t = pos.token();
					return t ? t->asRule() : null;
				}

				// See if this state is a Regex token.
				inline RegexToken *getRegex() const {
					Token *t = pos.token();
					return t ? t->asRegex() : null;
				}

				// See if this state finishes a production.
				inline Bool finishes(Production *p) const {
					return production() == p
						&& pos.end();
				}

				// Equality, as required by the parser. TODO: adhere to the future standard equal interface.
				inline Bool STORM_FN operator ==(const State &o) const {
					return pos == o.pos
						&& from == o.from;
				}

				inline Bool STORM_FN operator !=(const State &o) const {
					return !(*this == o);
				}
			};

			// To string.
			StrBuf &STORM_FN operator <<(StrBuf &to, State s);
			wostream &operator <<(wostream &to, State s);


			/**
			 * Ordered set of states. Supports proper ordering by priority.
			 */
			class StateSet : public Object {
				STORM_CLASS;
			public:
				// Create.
				StateSet();

				// Insert a state here if it does not exist. May alter the 'completed' member of an
				// existing state to obey priority rules.
				void STORM_FN push(Parser *parser, const State &state);

				// Insert a new state (we will allocate it for you if neccessary)
				void STORM_FN push(Parser *parser, ProductionIter pos, Nat from);
				void STORM_FN push(Parser *parser, ProductionIter pos, Nat from, StatePtr prev);
				void STORM_FN push(Parser *parser, ProductionIter pos, Nat from, StatePtr prev, StatePtr completed);

				// Current size of this set.
				inline Nat STORM_FN count() const {
					if (chunks == null)
						return 0;
					if (chunks->filled == 0)
						return 0;
					nat last = Nat(chunks->filled - 1);
					return last*chunkSize + Nat(chunks->v[last]->filled);
				}

				// Get an element in this set.
				inline const State &STORM_FN operator [](Nat i) const {
					GcArray<State> *chunk = chunks->v[i >> chunkBits];
					return chunk->v[i & chunkMask];
				}

			private:
				// Chunk size. Must be a power of two.
				enum {
					chunkBits = 5,
					chunkSize = (1 << chunkBits),
					chunkMask = chunkSize - 1
				};

				// State data. Represented as a two-level array to avoid copying.
				GcArray<GcArray<State> *> *chunks;

				// The GcType for arrays of State objects.
				const GcType *stateArrayType;

				// Size of the array to use when computing production ordering. Should be large enough
				// to cover the majority of productions, otherwise we loose performance.
				typedef PODArray<StatePtr, 20> StateArray;

				// Ordering.
				enum Order {
					before,
					after,
					none,
				};

				// Compute the execution order of two states when parsed by a top-down parser.
				Order execOrder(Parser *parser, const StatePtr &a, const StatePtr &b) const;
				Order execOrder(Parser *parser, const State &a, const State &b) const;

				// Find the 'completed' field for previous states. May contain entries representing null.
				void prevCompleted(Parser *parser, const State &from, StateArray &to) const;

				// Ensure we have at least 'n' chunks.
				void ensure(Nat n);
			};

		}
	}
}
