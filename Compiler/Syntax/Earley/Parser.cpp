#include "stdafx.h"
#include "Parser.h"
#include "Package.h"
#include "Compiler/Syntax/SStr.h"
#include "Utils/Memory.h"
#include "Core/Runtime.h"
#include "Lib/Parser.h"
#include "Compiler/Lib/Array.h"

namespace storm {
	namespace syntax {
		namespace earley {

			Parser::Parser() {
				rules = new (this) Map<Rule *, RuleInfo>();
				emptyString = new (this) Str(L"");
				lastFinish = -1;
			}

			Nat Parser::stateCount() const {
				Nat r = 0;
				if (steps)
					for (nat i = 0; i < steps->count; i++)
						r += steps->v[i]->count();
				return r;
			}

			Nat Parser::byteCount() const {
				nat stepCount = steps ? Nat(steps->count) : 0;
				return sizeof(Parser)
					+ stepCount * (sizeof(void *) + sizeof(StateSet))
					+ stateCount() * sizeof(State);
			}

			void Parser::add(Rule *rule) {
				// We only keep track of these to get better error messages!
				// Make sure to create an entry for the rule!
				rules->at(rule);
			}

			void Parser::add(ProductionType *t) {
				Rule *owner = as<Rule>(t->super());
				if (!owner)
					throw new (this) InternalError(TO_S(engine(), t->identifier() << S(" is not defined correctly.")));
				if (t->production->parent)
					throw new (this) SyntaxError(t->pos, S("Using parent productions is not supported by the Earley parser."));
				rules->at(owner).push(t->production);
			}

			static Bool find(Production *item, Array<Production *> *in) {
				for (Nat i = 0; i < in->count(); i++)
					if (in->at(i) == item)
						return true;
				return false;
			}

			static Bool compare(Array<Production *> *a, Array<Production *> *b) {
				Nat aCount = a ? a->count() : 0;
				Nat bCount = b ? b->count() : 0;
				if (aCount != bCount)
					return false;

				for (Nat i = 0; i < aCount; i++) {
					if (!find(a->at(i), b))
						return false;
				}

				return true;
			}

			Bool Parser::sameSyntax(ParserBackend *other) {
				if (runtime::typeOf(this) != runtime::typeOf(other))
					return false;
				Parser *o = (Parser *)other;

				for (Map<Rule *, RuleInfo>::Iter i = rules->begin(), end = rules->end(); i != end; i++) {
					RuleInfo ours = i.v();
					RuleInfo their = o->rules->get(i.k(), RuleInfo());
					if (!compare(ours.productions, their.productions))
						return false;
				}

				for (Map<Rule *, RuleInfo>::Iter i = o->rules->begin(), end = o->rules->end(); i != end; i++) {
					if (!rules->has(i.k())) {
						if (i.v().productions && i.v().productions->any())
							return false;
					}
				}

				return true;
			}

			void Parser::clear() {
				steps = null;
				src = null;
			}

			Bool Parser::parse(Rule *root, Str *str, Url *file, Str::Iter start) {
				// Remember what we parsed.
				src = str;
				Nat len = str->peekLength() + 1 - start.offset();
				lastFinish = len + 1;
				srcFile = file;
				srcOffset = start.offset();

				// Set up storage.
				Engine &e = engine();
				steps = runtime::allocArray<StateSet *>(e, &pointerArrayType, len);
				for (nat i = 0; i < len; i++)
					steps->v[i] = new (e) StateSet();

				// Create the initial production.
				rootProd = new (this) Production();
				rootProd->tokens->push(new (this) RuleToken(root, null));

				// Set up the initial state.
				steps->v[0]->push(this, rootProd->firstA(), 0);

				// Process all steps.
				for (nat i = 0; i < len; i++) {
					PARSER_PVAR(i);
					if (process(i))
						lastFinish = i;
				}

				return lastFinish < len;
			}

			bool Parser::process(nat step) {
				bool seenFinish = false;

				StateSet &src = *steps->v[step];
				for (nat i = 0; i < src.count(); i++) {
					StatePtr ptr(step, i);
					const State &at = src[i];
					PARSER_PVAR(at);

					predictor(ptr, at);
					completer(ptr, at);
					scanner(ptr, at);

					if (at.finishes(rootProd))
						seenFinish = true;
				}

				return seenFinish;
			}

			void Parser::predictor(StatePtr ptr, const State &state) {
				RuleToken *rule = state.getRule();
				if (!rule)
					return;

				// Note: this creates new (empty) entries for any rules referred to but not yet included.
				RuleInfo &info = rules->at(rule->rule);

				StateSet &src = *steps->v[ptr.step];
				if (info.productions) {
					for (Nat i = 0; i < info.productions->count(); i++) {
						Production *p = info.productions->at(i);
						src.push(this, p->firstA(), ptr.step);
						src.push(this, p->firstB(), ptr.step);
					}
				}

				if (matchesEmpty(info)) {
					// The original parser fails with rules like:
					// DELIMITER : " *";
					// Foo : "(" - DELIMITER - Bar - DELIMITER - ")";
					// Bar : DELIMITER;
					// since the completed state of Bar may already have been added and processed when
					// trying to match DELIMITER. Therefore, we need to look for completed instances of
					// Bar (since it may match "") in the current state before continuing. If it is not
					// found, it will be added and processed later, which is OK. This is basically what
					// the completer does (only less general):

					// We do not need to examine further than the current state. Anything else will be
					// examined in the main loop later.
					for (nat i = 0; src[i] != state && i < src.count(); i++) {
						const State &now = src[i];
						if (!now.pos.end())
							continue;

						// Only complete states that were actually instantiated here!
						if (now.from != ptr.step)
							continue;

						Rule *cRule = now.production()->rule();
						if (cRule != rule->rule)
							continue;

						src.push(this, state.pos.nextA(), state.from, ptr, StatePtr(ptr.step, i));
						src.push(this, state.pos.nextB(), state.from, ptr, StatePtr(ptr.step, i));
					}
				}
			}

			void Parser::completer(StatePtr ptr, const State &state) {
				if (!state.pos.end())
					return;

				Rule *completed = state.pos.rule();
				StateSet &from = *steps->v[state.from];
				for (nat i = 0; i < from.count(); i++) {
					const State &s = from[i];
					RuleToken *rule = s.getRule();
					if (!rule)
						continue;
					if (rule->rule != completed)
						continue;

					StatePtr prevPtr(state.from, i);
					steps->v[ptr.step]->push(this, s.pos.nextA(), s.from, prevPtr, ptr);
					steps->v[ptr.step]->push(this, s.pos.nextB(), s.from, prevPtr, ptr);
				}
			}

			void Parser::scanner(StatePtr ptr, const State &state) {
				RegexToken *regex = state.getRegex();
				if (!regex)
					return;

				nat matched = regex->regex.matchRaw(src, ptr.step + srcOffset);
				if (matched == Regex::NO_MATCH)
					return;

				// Should not happen, but better safe than sorry!
				if (matched < srcOffset)
					return;
				matched -= srcOffset;
				if (matched > steps->count)
					return;

				steps->v[matched]->push(this, state.pos.nextA(), state.from, ptr);
				steps->v[matched]->push(this, state.pos.nextB(), state.from, ptr);
			}

			bool Parser::matchesEmpty(Rule *rule) {
				return matchesEmpty(rules->at(rule));
			}

			bool Parser::matchesEmpty(RuleInfo &info) {
				if (info.matchesNull < 2)
					return info.matchesNull != 0;

				// Say the rule matches null in case it is recursive!
				// This will provide correct results and prevent endless recursion.
				info.matchesNull = 1;

				bool match = false;
				if (info.productions) {
					for (Nat i = 0; i < info.productions->count(); i++) {
						if (matchesEmpty(info.productions->at(i))) {
							match = true;
							break;
						}
					}
				}

				info.matchesNull = match ? 1 : 0;
				return match;
			}

			static const ProductionIter &pick(const ProductionIter &a, const ProductionIter &b) {
				if (!b.valid())
					return a;
				if (!a.valid())
					return b;
				if (a.position() < b.position())
					return b;
				else
					return a;
			}

			bool Parser::matchesEmpty(Production *p) {
				// Note: since we try to match against the empty string, we can greedily pick the one of
				// nextA and nextB which is furthest along the production at each step.
				ProductionIter pos = pick(p->firstA(), p->firstB());
				while (pos.valid() && !pos.end()) {
					if (!matchesEmpty(pos.token()))
						return false;

					pos = pick(pos.nextA(), pos.nextB());
				}

				return true;
			}

			bool Parser::matchesEmpty(Token *t) {
				if (RegexToken *r = t->asRegex()) {
					return r->regex.matchRaw(new (this) Str(L"")) != Regex::NO_MATCH;
				} else if (RuleToken *u = t->asRule()) {
					return matchesEmpty(u->rule);
				} else {
					assert(false, L"Unknown syntax token type.");
					return false;
				}
			}

			Bool Parser::hasError() const {
				if (!steps)
					return true;
				if (lastFinish < steps->count - 1)
					return true;

				return finish() == null;
			}

			Bool Parser::hasTree() const {
				return finish() != null;
			}

			Str::Iter Parser::matchEnd() const {
				if (finish())
					return src->posIter(srcOffset + lastFinish);
				else
					return src->begin();
			}

			Str *Parser::errorMsg() const {
				nat pos = lastStep();
				StrBuf *msg = new (this) StrBuf();

				if (!steps) {
					*msg << L"No parsing done.";
				} else if (pos == steps->count - 1) {
					*msg << L"Unexpected end of stream.";
				} else {
					Char ch(src->c_str()[pos + srcOffset]);
					Str *chStr = new (this) Str(ch);
					*msg << L"Unexpected '" << chStr->escape() << L"'.";
				}

				*msg << L"\nIn progress:";
				Indent z(msg);
				const StateSet &step = *steps->v[pos];
				for (nat i = 0; i < step.count(); i++) {
					*msg << L"\n" << step[i].pos;
				}

				return msg->toS();
			}

			SrcPos Parser::errorPos() const {
				Nat step = srcOffset + lastStep();
				return SrcPos(srcFile, step, step + 1);
			}

			nat Parser::lastStep() const {
				for (size_t i = steps->count - 1; i > 0; i--) {
					if (steps->v[i]->count() > 0)
						return nat(i);
				}

				// First step is never empty.
				return 0;
			}

			const State *Parser::finish() const {
				if (lastFinish >= steps->count)
					return null;

				const StateSet &states = *steps->v[lastFinish];
				for (nat i = 0; i < states.count(); i++) {
					const State &s = states[i];
					if (s.finishes(rootProd))
						return &s;
				}

				return null;
			}

			const State &Parser::state(const StatePtr &ptr) const {
				return (*steps->v[ptr.step])[ptr.index];
			}

			Node *Parser::tree() const {
				const State *from = finish();
				if (!from)
					return null;

				// 'from' finishes the dummy 'rootProd', which the user is not interested in. The user
				// is interested in the production that finished 'rootProd', which is located in
				// 'from->completed'.
				assert(from->completed != StatePtr(), L"The root state was not completed by anything!");
				return tree(from->completed);
			}

			// See so that 'offset' is exposed as a GC:d pointer.
			static inline void checkOffset(Node *node, int offset) {
#ifdef SLOW_DEBUG
				const GcType *gc = runtime::gcTypeOf(node);
				for (nat i = 0; i < gc->count; i++)
					if (offset == gc->offset[i])
						return;

				PLN(L"The offset " << offset << L" is not present for " << ::toS(gc->type));
				PLN(L"Variables: " << ::toS(gc->type->variables()));
				PLN(L"Offsets present for this type:");
				PLN(L"Size: " << gc->stride);
				for (nat i = 0; i < gc->count; i++)
					PLN(i << L": " << gc->offset[i]);

				dbg_assert(false, L"");
#endif
			}

			template <class T>
			static void setValue(Node *node, MemberVar *target, T *elem) {
				int offset = target->offset().current();
				checkOffset(node, offset);
				if (isArray(target->type)) {
					// Arrays are initialized earlier.
					OFFSET_IN(node, offset, Array<T *> *)->push(elem);
				} else {
					OFFSET_IN(node, offset, T *) = elem;
				}
			}

			Node *Parser::tree(StatePtr fromPtr) const {
				const State &from = state(fromPtr);
				Node *result = allocNode(from, fromPtr.step);
				Production *prod = from.production();

				// Remember capture start and capture end. Set 'repStart' to first token since we will
				// not find it in the loop if the repeat starts at the first token in this production.
				nat repStart = from.from;
				nat repEnd = 0;

				// Traverse the states backwards. The last token in the chain (the one created first) is
				// skipped as that does not contain any information.
				StatePtr atPtr = fromPtr;
				const State *at = &from;
				while (at->prev != StatePtr()) {
					const State *prev = &state(at->prev);
					Token *token = prev->pos.token();

					if (at->pos.repStart())
						repStart = atPtr.step;
					if (at->pos.repEnd())
						repEnd = atPtr.step;

					if (token->target) {
						if (token->asRegex()) {
							Str::Iter from = src->posIter(at->prev.step + srcOffset);
							Str::Iter to = src->posIter(atPtr.step + srcOffset);

							SrcPos pos(srcFile, srcOffset + repStart, srcOffset + repEnd);
							setValue(result, token->target, new (this) SStr(src->substr(from, to), pos));
						} else if (token->asRule()) {
							assert(at->completed != StatePtr(), L"Rule token not completed!");
							setValue(result, token->target, tree(at->completed));
						} else {
							assert(false, L"Unknown token type used for match.");
						}
					}

					atPtr = at->prev;
					at = prev;
				}

				// Remember the capture.
				if (prod->repCapture && prod->repCapture->target && repStart <= repEnd) {
					Str::Iter from = src->posIter(repStart + srcOffset);
					Str::Iter to = src->posIter(repEnd + srcOffset);

					SrcPos pos(srcFile, srcOffset + repStart, srcOffset + repEnd);
					setValue(result, prod->repCapture->target, new (this) SStr(src->substr(from, to), pos));
				}

				// Reverse all arrays in this node, as we're adding them backwards.
				reverseNode(result);
				return result;
			}

			Node *Parser::allocNode(const State &from, Nat endStep) const {
				ProductionType *type = from.production()->type();

				// A bit ugly, but this is enough for the object to be considered a proper object when
				// it is populated.
				void *mem = runtime::allocObject(type->size().current(), type);
				Node *r = new (Place(mem)) Node(SrcPos(srcFile, srcOffset + from.from, srcOffset + endStep));
				type->vtable()->insert(r);

				// Create any arrays needed.
				for (nat i = 0; i < type->arrayMembers->count(); i++) {
					MemberVar *v = type->arrayMembers->at(i);
					int offset = v->offset().current();
					checkOffset(r, offset);
					// This will actually create the correct subtype as long as we're creating something
					// inherited from Object (which we are).
					Array<Object *> *arr = new (v->type.type) Array<Object *>();
					OFFSET_IN(r, offset, Object *) = arr;
				}

				return r;
			}

			void Parser::reverseNode(Node *node) const {
				ProductionType *t = as<ProductionType>(runtime::typeOf(node));
				if (!t) {
					WARNING(L"Invalid node type found!");
					return;
				}

				for (nat i = 0; i < t->arrayMembers->count(); i++) {
					MemberVar *v = t->arrayMembers->at(i);
					int offset = v->offset().current();

					Array<Object *> *array = OFFSET_IN(node, offset, Array<Object *> *);
					array->reverse();
				}
			}

			InfoNode *Parser::infoTree() const {
				// TODO: Make this robust in case of parser errors!
				const State *from = finish();
				if (!from)
					return null;

				assert(from->completed != StatePtr(), L"The root state was not completed by anything!");
				return infoTree(from->completed);
			}

			InfoNode *Parser::infoTree(StatePtr endPtr) const {
				const State &end = state(endPtr);

				// Compute the number of child nodes required for this internal node.
				Nat children = 0;
				StatePtr atPtr = endPtr;
				const State *at = &end;
				while (at->prev != StatePtr()) {
					children++;

					atPtr = at->prev;
					at = &state(at->prev);
				}

				// Allocate the node and fill it!
				Production *p = end.pos.production();
				InfoInternal *result = new (this) InfoInternal(p, children);
				if (p->indentType != indentNone)
					result->indent = new (this) InfoIndent(0, children, p->indentType);

				atPtr = endPtr;
				at = &end;
				while (at->prev != StatePtr()) {
					const State *prev = &state(at->prev);
					Token *token = prev->pos.token();

					if (result->indent) {
						if (at->pos.position() == p->indentStart)
							result->indent->start = children;
						else if (at->pos.position() == p->indentEnd)
							result->indent->end = children;
					}

					InfoNode *child = null;
					if (at->completed != StatePtr()) {
						child = infoTree(at->completed);
						if (token->asDelim())
							child->delimiter(true);
					} else {
						Str::Iter from = src->posIter(at->prev.step + srcOffset);
						Str::Iter to = src->posIter(atPtr.step + srcOffset);
						Str *s = emptyString;
						if (from != to)
							s = src->substr(from, to);
						child = new (this) InfoLeaf(token->asRegex(), s);
					}

					child->color = token->color;
					result->set(--children, child);

					atPtr = at->prev;
					at = prev;
				}

				return result;
			}


			/**
			 * Rule info struct.
			 */

			Parser::RuleInfo::RuleInfo() : productions(null), matchesNull(2) {}

			void Parser::RuleInfo::push(Production *p) {
				if (!productions)
					productions = new (p) Array<Production *>();
				productions->push(p);
			}

		}
	}
}
