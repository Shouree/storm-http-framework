#include "stdafx.h"
#include "Item.h"
#include "Exception.h"

namespace storm {
	namespace syntax {
		namespace glr {

			/**
			 * Item.
			 */

			Item::Item(Syntax *world, Production *p) {
				id = world->lookup(p);
				pos = firstPos(p);
			}

			Item::Item(Syntax *world, Nat production) {
				id = production;
				switch (Syntax::specialProd(production)) {
				case 0:
					pos = firstPos(world->production(production));
					break;
				case Syntax::prodEpsilon:
				case Syntax::prodESkip:
					pos = endPos;
					break;
				case Syntax::prodRepeat:
					pos = firstRepPos(world->production(production));
					break;
				}
			}

			Item::Item(Nat id, Nat pos) : id(id), pos(pos) {}

			Item last(Nat production) {
				return Item(production, Item::endPos);
			}

			Item special(Nat production) {
				return Item(production, Item::specialPos);
			}

			Nat Item::endPos = -1;
			Nat Item::specialPos = -2;

			Nat Item::firstPos(Production *p) {
				if (skippable(p->repType) && p->repStart == 0)
					return specialPos;
				else if (p->tokens->empty())
					return endPos;
				else
					return 0;
			}

			Nat Item::nextPos(Production *p, Nat pos) {
				if (pos == specialPos) {
					if (p->repEnd >= p->tokens->count())
						return endPos;
					else
						return p->repEnd;
				} else if (skippable(p->repType)) {
					if (pos + 1 == p->repStart)
						return specialPos;
				} else if (p->repType != repNone) {
					if (pos + 1 == p->repEnd)
						return specialPos;
				}

				if (pos + 1 >= p->tokens->count())
					return endPos;
				else
					return pos + 1;
			}

			Bool Item::prevPos(Production *p, Nat &pos) {
				if (pos == endPos) {
					pos = p->tokens->count();
					if (pos == 0) {
						pos = endPos;
						return false;
					}
					if (pos == p->repEnd && p->repType != repNone)
						pos = specialPos;
					else
						pos--;
					return true;
				} else if (pos == specialPos) {
					pos = skippable(p->repType) ? p->repStart : p->repEnd;
					if (pos == 0) {
						pos = specialPos;
						return false;
					}
					pos--;
					return true;
				} else {
					if (pos == 0)
						return false;
					if (pos-- == p->repEnd && p->repType != repNone)
						pos = specialPos;
					return true;
				}
			}

			Nat Item::firstRepPos(Production *p) {
				if (repeatable(p->repType))
					return specialPos;
				else if (p->repStart == p->repEnd)
					return endPos;
				else
					return p->repStart;
			}

			Nat Item::nextRepPos(Production *p, Nat pos) {
				if (pos == specialPos)
					return p->repStart;

				if (pos + 1 >= p->repEnd)
					return endPos;
				else
					return pos + 1;
			}

			Bool Item::prevRepPos(Production *p, Nat &pos) {
				if (pos == endPos) {
					pos = p->repEnd;
					if (pos == p->repStart)
						pos = specialPos;
					else
						pos--;
					return true;
				} else if (pos == specialPos) {
					// This is always the first token if we ever get here.
					return false;
				} else {
					if (pos == p->repStart)
						if (repeatable(p->repType))
							pos = specialPos;
						else
							return false;
					else
						pos--;
					return true;
				}
			}

			Item Item::next(Syntax *syntax) const {
				if (pos == endPos)
					return Item(id, endPos);
				if (Syntax::specialProd(id) == Syntax::prodESkip)
					return Item(id, endPos);

				return next(syntax->production(id));
			}

			Item Item::next(Production *p) const {
				if (pos == endPos)
					return Item(id, endPos);

				switch (Syntax::specialProd(id)) {
				case 0:
					return Item(id, nextPos(p, pos));
				case Syntax::prodEpsilon:
				case Syntax::prodESkip:
					return Item(id, endPos);
				case Syntax::prodRepeat:
					return Item(id, nextRepPos(p, pos));
				}

				return Item(id, endPos);
			}

			Bool Item::prev(Syntax *syntax) {
				if (Syntax::specialProd(id) == Syntax::prodESkip)
					return false;

				return prev(syntax->production(id));
			}

			Bool Item::prev(Production *p) {
				switch (Syntax::specialProd(id)) {
				case 0:
					return prevPos(p, pos);
				case Syntax::prodEpsilon:
				case Syntax::prodESkip:
					return false;
				case Syntax::prodRepeat:
					return prevRepPos(p, pos);
				}

				return false;
			}

			Nat Item::rule(Syntax *syntax) const {
				switch (Syntax::specialProd(id)) {
				case 0:
				default:
					return syntax->lookup(syntax->production(Syntax::baseProd(id))->rule());
				case Syntax::prodEpsilon:
				case Syntax::prodRepeat:
					return Syntax::baseProd(id) | Syntax::ruleRepeat;
				case Syntax::prodESkip:
					return Syntax::baseProd(id) | Syntax::ruleESkip;
				}
			}

			Nat Item::length(Syntax *syntax) const {
				Nat special = Syntax::specialProd(id);
				if (special == Syntax::prodESkip || special == Syntax::prodEpsilon)
					return 0;

				return length(syntax->production(id));
			}

			Nat Item::length(Production *p) const {
				Nat special = Syntax::specialProd(id);
				if (special == Syntax::prodESkip || special == Syntax::prodEpsilon)
					return 0;

				Nat rep = p->repEnd - p->repStart;
				if (special == Syntax::prodRepeat) {
					if (repeatable(p->repType))
						return rep + 1;
					else
						return rep;
				} else {
					if (skippable(p->repType))
						return p->tokens->count() - rep + 1;
					else if (p->repType == repNone)
						return p->tokens->count();
					else
						return p->tokens->count() + 1;
				}
			}

			Nat Item::tokenPos(Syntax *syntax) const {
				if (pos == endPos)
					return length(syntax);

				switch (Syntax::specialProd(id)) {
				case 0:
				case Syntax::prodRepeat:
					return tokenPos(syntax->production(id));
				case Syntax::prodEpsilon:
				case Syntax::prodESkip:
				default:
					return 0;
				}
			}

			Nat Item::tokenPos(Production *p) const {
				if (pos == endPos)
					return length(p);

				switch (Syntax::specialProd(id)) {
				case 0:
					if (pos == specialPos)
						return p->repStart;
					else if (p->repType != repNone && pos >= p->repEnd)
						return p->repStart + 1 + pos - p->repEnd;
					else
						return pos;
				case Syntax::prodRepeat:
					if (pos == specialPos)
						return 0;
					else
						return pos + 1 - p->repStart;
				case Syntax::prodEpsilon:
				case Syntax::prodESkip:
				default:
					return 0;
				}
			}

			Bool Item::end() const {
				return pos == endPos;
			}

			Bool Item::isRule(Syntax *syntax) const {
				if (end())
					return false;
				if (pos == specialPos)
					return true;

				Production *p = syntax->production(id);
				return p->tokens->at(pos)->asRule() != null;
			}

			Nat Item::nextRule(Syntax *syntax) const {
				assert(!end());
				assert(isRule(syntax));

				if (pos == specialPos)
					return Syntax::baseProd(id) | Syntax::ruleRepeat;

				Production *p = syntax->production(id);
				RuleToken *r = p->tokens->at(pos)->asRule();
				return syntax->lookup(r->rule);
			}

			Regex Item::nextRegex(Syntax *syntax) const {
				assert(!end());
				assert(!isRule(syntax));

				Production *p = syntax->production(id);
				return p->tokens->at(pos)->asRegex()->regex;
			}

			Bool Item::regexBefore(Syntax *syntax) const {
				Item i = *this;
				while (i.prev(syntax)) {
					if (!i.isRule(syntax)) {
						// This can not be the end, so it has to be a regex!
						return true;
					}
				}

				return false;
			}

			Nat Item::hash() const {
				return id + pos + (id << 8);
			}

			void Item::outputToken(StrBuf *to, Production *p, Nat pos) const {
				if (pos == specialPos) {
					*to << p->rule()->identifier() << S("'");
				} else {
					p->tokens->at(pos)->toS(to, false);
				}
			}

			void Item::output(StrBuf *to, Production *p, Nat mark, FirstFn first, NextFn next) const {
				bool prevDelim = false;
				Nat firstId = (*first)(p);
				for (Nat i = firstId; i != endPos; i = (*next)(p, i)) {
					bool currentDelim = false;
					if (i < p->tokens->count())
						currentDelim = p->tokens->at(i)->asDelim() != null;

					if (i != firstId && !currentDelim && !prevDelim)
						*to << S(" - ");

					if (i == mark)
						*to << S("<>");

					if (currentDelim)
						*to << S(", ");
					else
						outputToken(to, p, i);

					prevDelim = currentDelim;
				}
			}

			Str *Item::toS(Syntax *syntax) const {
				StrBuf *to = new (syntax) StrBuf();

				switch (Syntax::specialProd(id)) {
				case 0:
					*to << id;
					break;
				case Syntax::prodRepeat:
					*to << Syntax::baseProd(id) << S("'");
					break;
				case Syntax::prodEpsilon:
					*to << Syntax::baseProd(id) << S("''");
					break;
				case Syntax::prodESkip:
					*to << S("red") << Syntax::baseProd(id) << S(" ->");
					return to->toS();
				}
				*to << S(": ");

				Production *p = syntax->production(id);
				*to << p->rule()->identifier();
				if (Syntax::specialProd(id))
					*to << S("'");
				else if (p->priority != 0)
					*to << S("[") << p->priority << S("]");
				*to << S(" -> ");

				switch (Syntax::specialProd(id)) {
				case 0:
					output(to, p, pos, &Item::firstPos, &Item::nextPos);
					break;
				case Syntax::prodRepeat:
					output(to, p, pos, &Item::firstRepPos, &Item::nextRepPos);
					break;
				}

				if (pos == endPos)
					*to << S("<>");

				return to->toS();
			}

			void Item::toS(StrBuf *to) const {
				*to << S("(") << id << S(", ") << pos << S(")");
			}


			/**
			 * Item set.
			 */

			ItemSet::ItemSet() : data(null) {}

			Item ItemSet::operator [](Nat id) const {
				assert(id < count(), L"Index out of bounds!");
				return at(id);
			}

			Bool ItemSet::operator ==(ItemSet o) const {
				Nat c = count();
				if (c != o.count())
					return false;

				for (Nat i = 0; i < c; i++)
					if (data->v[i] != o.data->v[i])
						return false;

				return true;
			}

			Bool ItemSet::operator !=(ItemSet o) const {
				return !(*this == o);
			}

			Bool ItemSet::has(Item i) const {
				Nat p = itemPos(i);
				if (p >= count())
					return false;
				return data->v[p] == i;
			}

			Nat ItemSet::hash() const {
				Nat r = 5381;
				Nat c = count();
				for (Nat i = 0; i < c; i++) {
					r = ((r << 5) + r) + data->v[i].hash();
				}
				return r;
			}

			static const GcType itemType = {
				GcType::tArray,
				null,
				null,
				sizeof(Item),
				0,
				{},
			};

			Nat ItemSet::itemPos(Item find) const {
				Nat from = 0;
				Nat to = count();
				while (from < to) {
					nat mid = (to - from) / 2 + from;

					if (data->v[mid] < find) {
						from = mid + 1;
					} else {
						to = mid;
					}
				}

				return from;
			}

			Bool ItemSet::push(Engine &e, Item item) {
				// Find insertion position.
				Nat c = count();
				Nat insert = itemPos(item);

				// Already present?
				if (insert < c && item == data->v[insert])
					return false;

				// Insert and move the others.
				if (!data || data->filled == data->count) {
					// New allocation is needed.
					GcArray<Item> *alloc = runtime::allocArray<Item>(e, &itemType, c + grow);

					alloc->v[insert] = item;
					if (data) {
						alloc->filled = data->filled;
						memcpy(alloc->v, data->v, sizeof(Item)*insert);
						memcpy(alloc->v + insert + 1, data->v + insert, sizeof(Item)*(c - insert));
					}
					data = alloc;
				} else {
					memmove(data->v + insert + 1, data->v + insert, sizeof(Item)*(c - insert));
					data->v[insert] = item;
				}

				data->filled++;
				return true;
			}

			static void expand(Engine &e, ItemSet &to, const Item &i, Syntax *syntax) {
				if (!to.push(e, i))
					return;

				if (!i.isRule(syntax))
					return;

				Nat rule = i.nextRule(syntax);
				RuleInfo *info = syntax->ruleInfo(rule);
				for (Nat i = 0; i < info->count(); i++) {
					expand(e, to, Item(syntax, info->at(i)), syntax);
				}
			}

			ItemSet ItemSet::expand(Syntax *syntax) const {
				ItemSet result;
				Engine &e = syntax->engine();

				for (Nat i = 0; i < count(); i++) {
					glr::expand(e, result, at(i), syntax);
				}

				return result;
			}

			Str *ItemSet::toS(Syntax *syntax) const {
				StrBuf *to = new (syntax) StrBuf();
				*to << L"{\n";
				{
					Indent z(to);
					for (Nat i = 0; i < count(); i++) {
						*to << at(i) << L"\n";
					}
				}
				*to << L"}";
				return to->toS();
			}

			Bool push(EnginePtr e, ItemSet &to, Item item) {
				return to.push(e.v, item);
			}

			void ItemSet::toS(StrBuf *to) const {
				*to << S("{\n");
				{
					Indent z(to);
					for (Nat i = 0; i < count(); i++) {
						*to << at(i) << S("\n");
					}
				}
				*to << S("}");
			}

		}
	}
}
