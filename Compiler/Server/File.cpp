#include "stdafx.h"
#include "File.h"
#include "Engine.h"

namespace storm {
	namespace server {
		using namespace storm::syntax;

		Node::Node(InfoInternal *node, Range range) : node(node), range(range) {}

		void Node::toS(StrBuf *to) const {
			*to << range << S(", length ") << node->length();
		}

		static Nat strlen(Str *s) {
			Nat len = 0;
			for (Str::Iter i = s->begin(), e = s->end(); i != e; ++i)
				len++;
			return len;
		}

		Part::Part(File *owner, Nat offset, FileReader *part, MAYBE(FileReader *) next) {
			this->owner = owner;
			start = offset;
			path = part->info->url;

			Str *src = part->info->contents;
			Str::Iter expectedFinish = src->end();
			if (next)
				expectedFinish = next->info->start;
			src = src->substr(part->info->start, expectedFinish);

			content = new (this) InfoLeaf(null, src);
			parser = part->createParser();
			if (InfoNode *n = parse(content, parser->root()))
				content = n;
		}

		Bool Part::replace(FileReader *reader, MAYBE(FileReader *) next, Bool force) {
			InfoParser *p = reader->createParser();
			if (!force && parser->sameSyntax(p))
				return false;

			Str *src = reader->info->contents;
			Str::Iter expectedFinish = src->end();
			if (next)
				expectedFinish = next->info->start;
			src = src->substr(reader->info->start, expectedFinish);

			InfoNode *c = new (this) InfoLeaf(null, src);
			parser = p;
			InfoErrors pResult;
			if (InfoNode *n = parse(c, parser->root(), &pResult)) {
				content = n;
				// if (!bad(pResult, n)) {
				// 	content = n;
				// } else if (n->length() != content->length()) {
				// 	// Length changed, we have no choice but keeping the bad version...
				// 	content = n;
				// }
			}

			return true;
		}

		void Part::adjustOffset(Nat offset, Str *src) {
			if (offset > start) {
				// Remove some data. TODO: Consider what happens when we want to contain less than
				// zero characters.
				remove(Range(start, offset));
			} else if (offset < start) {
				// Insert some data.
				Str::Iter s = src->posIter(offset);
				Str::Iter e = src->posIter(start);
				insert(start, src->substr(s, e));
			}

			this->offset(offset);
		}

		Range Part::full() const {
			return Range(start, start + content->length());
		}

		Range Part::replace(Range range, Str *replace) {
			if (!content)
				return Range(0, 0);
			// PLN(TO_S(this, L"Replace " << range << L" with " << replace << L", len " << content->length()));

			// Do all regexes match?
			Bool regexMatching = true;

			// Anything to remove?
			if (!range.empty()) {
				regexMatching &= remove(range);
				range = Range(range.from, range.from);
			}

			// Anything to add?
			if (replace->begin() != replace->end()) {
				regexMatching &= insert(range.from, replace);
				range = Range(range.from, range.from + replace->peekLength());
			}

			Range result(range.from, range.from + strlen(replace));
			if (!regexMatching) {
				result = merge(result, parse(range));
			}

			return result;
		}

		Bool Part::remove(const Range &range) {
			return remove(range, start, content, false);
		}

		Bool Part::remove(const Range &range, Nat offset, InfoNode *node, Bool seenError) {
			Nat len = node->length();
			if (len == 0)
				return true;

			Range nodeRange(offset, offset + len);
			if (!nodeRange.intersects(range))
				return true;

			if (node->error())
				seenError = true;

			if (InfoLeaf *leaf = as<InfoLeaf>(node)) {
				return removeLeaf(range, offset, leaf, seenError);
			} else if (InfoInternal *inode = as<InfoInternal>(node)) {
				return removeInternal(range, offset, inode, seenError);
			} else {
				assert(false, L"This should not happen!");
				return false; // Force a re-parse.
			}
		}

		Bool Part::removeLeaf(const Range &range, Nat offset, InfoLeaf *node, Bool seenError) {
			Str *src = node->toS();
			Str::Iter begin = src->begin();
			for (Nat i = offset; i < range.from; i++)
				++begin;

			Str::Iter end = begin;
			for (Nat i = max(offset, range.from); i < range.to; i++)
				++end;

			node->set(src->remove(begin, end));

			return !seenError && node->matchesRegex();
		}

		Bool Part::removeInternal(const Range &range, Nat offset, InfoInternal *node, Bool seenError) {
			Bool ok = true;

			for (Nat i = 0; i < node->count(); i++) {
				InfoNode *at = node->at(i);
				// Keep track of the old length, as that will likely change.
				Nat len = at->length();
				ok &= remove(range, offset, at, seenError);
				offset += len;
			}

			return ok;
		}

		struct Part::InsertState {
			// Insert 'v' at 'point'.
			Nat point;
			Str *v;

			// Anything inserted so far?
			Bool done;
			// Was there an error node at the path to the insertion point?
			Bool error;

			// First viable match. Maybe not chosen due to regex mismatch.
			InfoLeaf *firstPossible;
			Str *firstPossibleStr;
		};

		Bool Part::insert(Nat point, Str *v) {
			InsertState state = {
				point,
				v,
				false,
				false,
				null,
				null,
			};
			insert(state, start, content, false);

			// If 'done' is true, then we succeeded at inserting the string somewhere where the
			// regex matched.
			if (state.done)
				return !state.error;

			// We should insert it at 'firstPossible'. If it is null, we have a syntax tree without
			// leaves, which can happen when the grammar contains epsilon-productions.
			if (!state.firstPossible) {
				// If so: replace the entire tree since we know it is empty anyway.
				content = new (this) InfoLeaf(null, v);
				// Force a re-parse.
				return false;
			}

			// We can not avoid invalidating a regex...
			state.firstPossible->set(state.firstPossibleStr);
			return false;
		}

		void Part::insert(InsertState &state, Nat offset, InfoNode *node, Bool seenError) {
			// Outside this node?
			Nat len = node->length();
			if (state.point < offset || state.point > offset + len)
				return;

			if (node->error())
				seenError = true;

			if (InfoLeaf *leaf = as<InfoLeaf>(node)) {
				insertLeaf(state, offset, leaf, seenError);
			} else if (InfoInternal *inode = as<InfoInternal>(node)) {
				insertInternal(state, offset, inode, seenError);
			} else {
				assert(false, L"This should not happen!");
			}
		}

		void Part::insertLeaf(InsertState &state, Nat offset, InfoLeaf *node, Bool seenError) {
			Str *src = node->toS();
			Str::Iter p = src->begin();
			for (Nat i = offset; i < state.point; i++)
				++p;

			Str *modified = src->insert(p, state.v);
			if (!state.firstPossible) {
				state.firstPossible = node;
				state.firstPossibleStr = modified;
			}

			if (node->matchesRegex(modified)) {
				node->set(modified);
				state.done = true;
				state.error = seenError;
			}
		}

		void Part::insertInternal(InsertState &state, Nat offset, InfoInternal *node, Bool seenError) {
			for (Nat i = 0; i < node->count() && !state.done; i++) {
				InfoNode *at = node->at(i);
				Nat len = at->length();
				insert(state, offset, at, seenError);
				offset += len;
			}
		}

		TextIndent Part::indent(Nat pos) {
			if (pos < start || !content)
				return TextIndent();

			TextIndent r = content->indentAt(pos - start);
			r.offset(start);
			return r;
		}

		Array<ColoredRange> *Part::colors(Range range) {
			Array<ColoredRange> *r = new (this) Array<ColoredRange>();
			if (content)
				colors(r, range, start, content);
			return r;
		}

		void Part::colors(Array<ColoredRange> *out, const Range &range, Nat offset, InfoNode *node) {
			// We can ignore zero-length nodes.
			Nat len = node->length();
			if (len == 0)
				return;

			Range nodeRange(offset, offset + len);
			if (!nodeRange.intersects(range))
				return;

			// Pick the root-most color.
			if (node->color != syntax::tNone) {
				out->push(ColoredRange(nodeRange, node->color));
				return;
			}

			// Else, continue traversing.
			if (InfoInternal *n = as<InfoInternal>(node)) {
				Nat nodeOffset = offset;
				for (Nat i = 0; i < n->count(); i++) {
					colors(out, range, nodeOffset, n->at(i));
					nodeOffset += n->at(i)->length();
				}
			}
		}

		InfoNode *Part::parse(InfoNode *node, Rule *root) {
			return parse(node, root, null);
		}

		InfoNode *Part::parse(InfoNode *node, Rule *root, MAYBE(InfoErrors *) out) {
			parser->root(root);
			Str *src = node->toS();

			// Note: It's actually fine to pass NULL as a context, even if the interface does not say so.
			InfoErrors e = parser->parseApprox(src, path, node->parent());
			if (out)
				*out = e;

			// PLN(TO_S(this, L"Parsed " << src->peekLength() << L" - " << root->identifier() << L" - " << e));
			// if (src->peekLength() < 50)
			// 	PLN(src->escape());

			InfoNode *r = null;
			if (parser->hasTree() && parser->matchEnd() == src->end()) {
				// We managed to match something using error recovery!
				// TODO: Return info indicating the quality of the match.
				r = parser->infoTree();
			} else if (node->parent() == null) {
				// The root node failed. Output as much as possible at least...
				r = parser->fullInfoTree();
			}

			parser->clear();
			return r;
		}

		Range Part::parse(const Range &range) {
			// PLN(TO_S(this, L"----- PARSING " << range << L" -----"));
			Array<Node> *path = findParsePath(range, start, content);

			if (!path || path->empty()) {
				// No relevant node was found. Invalidate the border towards the previous part and
				// let a more aggressive re-parse do the work.
				owner->postInvalidate(this, File::prevBorder, true);
				return Range();
			}

			// Traverse the node upwards and pick one to re-parse.
			Range altered = parsePath(path, range);

			if (altered.empty()) {
				// Re-parse from the start.
				owner->postInvalidate(this, -1, true);
			}

			return altered;

			// TODO: We should keep track of ranges where the error correction kicked in and keep
			// that in mind when re-parsing stuff. We could also post a job for re-parsing those
			// chunks in the background.
		}

		Array<Node> *Part::findParsePath(const Range &update, Nat offset, InfoNode *node) {
			Nat len = node->length();
			if (len == 0)
				return null;

			// Does this node completely cover 'range'?
			Range nodeRange(offset, offset + len);
			if (nodeRange.from > update.from || nodeRange.to < update.to)
				return null;

			// Do not attempt to re-parse leaf nodes as they only contain regexes which have already
			// been checked.
			InfoInternal *inode = as<InfoInternal>(node);
			if (!inode)
				return null;

			// Look at the children of this node.
			Array<Node> *result = null;
			for (Nat i = 0; i < inode->count(); i++) {
				InfoNode *child = inode->at(i);
				Array<Node> *r = findParsePath(update, offset, child);
				offset += child->length();

				if (r) {
					if (!result) {
						// First viable child. Use that!
						result = r;
					} else {
						// More than one possible. Start from here instead.
						result = null;
						break;
					}
				}
			}

			// No relevant child?
			if (!result)
				result = new (this) Array<Node>();

			// Add ourself.
			result->push(Node(inode, nodeRange));

			return result;
		}

		static void widen(Range &range, Nat val) {
			if (range.from > val)
				range.from -= val;
			else
				range.from = 0;
			range.to += val;
		}

		Range Part::parsePath(Array<Node> *path, Range target) {
			assert(path->any(), L"The path should contain at least one node!");
			Range start = path->at(0).range;
			Nat targetLen = start.count() + reparseExtra;
			widen(target, reparseEdge);

			// We have two constraints: first, the node we are parsing should be long enough (as
			// denoted by targetLen), and secondly we should include some additional text on both
			// sides of the edit (or any errors we find along the way). This helps us from getting
			// stuck in locally optimal solutions on subsequent re-parses.

			Nat chosen = 0;
			Bool found = false;
			for (Nat i = 1; i < path->count(); i++) {
				const Node &now = path->at(i);
				Nat count = now.range.count();

				// Ignore too large nodes so that we do not degrade performance too much.
				if (count > reparseMax)
					break;

				// We reached our target. Now we're happy!
				if (count >= targetLen
					&& now.range.from <= target.from
					&& now.range.to >= target.to
					&& !found) {

					chosen = i;
					found = true;
				}

				// Re-parse more if we find error nodes somewhere.
				if (now.node->error() || now.node->delimiter()) {
					found = false;
					target = now.range;
					targetLen = target.count() + reparseExtra;
					widen(target, reparseEdge);
				}

				if (!found)
					chosen = i;
			}

			// If the first candidate was too large, we most likely need a large-scale reparse at this point.
			if (!found && chosen == 0)
				owner->postInvalidate(this, 0, true);

			// Ignore nodes without production as far as possible.
			while (chosen < path->count() && !path->at(chosen).node->production())
				chosen++;

			// No production here?
			Node n = path->at(chosen);
			if (!n.node->production())
				return Range();

			// Any errors in the parent nodes?
			for (Nat i = chosen + 1; i < path->count(); i++) {
				if (path->at(i).node->error()) {
					owner->postInvalidate(this, 0, true);
					// No need to look further.
					break;
				}
			}

			// Re-parse 'n'!
			InfoErrors errors;
			InfoNode *result = parse(n.node, n.node->production()->rule(), &errors);

			// No result. Report failure!
			if (!result)
				return Range();

			// We got a result. Use it!
			replace(n.node, result);
			// PLN(TO_S(this, L"Replace " << n.node->production()->rule()->name << L" at " << n.range << L": " << errors));

			// If the result was bad (ie. required lots of error recovery), use it in the meantime,
			// but attempt to generate a better syntax tree later on by scheduling a full re-parse.
			if (bad(errors, n.node)) {
				// Invalidate us and the previous part. This correction was very bad, so we should
				// probably re-parse a lot.
				owner->postInvalidate(this, -1, true);
			} else {
				// TODO: Be a bit more conservative?
				if (n.range.from <= full().from + invalidateLength) {
					// Invalidate the border towards the previous part.
					owner->postInvalidate(this, File::prevBorder, true);
				} else if (n.range.to >= full().to - invalidateLength) {
					// Invalidate the border towards the next part.
					owner->postInvalidate(this, File::nextBorder, true);
				}
			}

			// Report success.
			return n.range;
		}

		bool Part::bad(InfoErrors w, InfoNode *node) {
			if (!w.success())
				return true;

			Nat len = node->length();
			// NOTE: Consider the case when 'len' == 0.
			if (w.chars() > len * 0.6)
				return true;
			if (w.shifts() > max(15.0, len*0.1))
				return true;

			return false;
		}

		InfoErrors Part::combine(InfoErrors a, InfoErrors b) {
			if (!a.success() || !b.success())
				return infoFailure();

			return a + b;
		}

		void Part::replace(InfoNode *oldNode, InfoNode *newNode) {
			InfoInternal *parent = oldNode->parent();
			if (!parent) {
				assert(oldNode == content, L"A non-root node without a parent found.");
				content = newNode;
			} else {
				for (Nat i = 0; i < parent->count(); i++) {
					if (parent->at(i) == oldNode) {
						parent->set(i, newNode);
						return;
					}
				}

				assert(false, L"The child node was not present in our parent.");
			}
		}

		void Part::debugOutput(TextOutput *to, Bool tree) const {
			if (tree) {
				to->writeLine(content->format());
			} else {
				to->writeLine(content->toS());
			}
		}

		Nat Part::dbg_size() const {
			return content->dbg_size();
		}

		void Part::text(StrBuf *to) const {
			content->toS(to);
		}


		/**
		 * File.
		 */

		File::File(Nat id, Url *path, Str *src, WorkQueue *q)
			: id(id), path(path), work(q) {

			Engine &e = engine();
			emptyStr = new (e) Str();
			parts = new (e) Array<Part *>();
			package = e.package(path->parent());
			if (!package) {
				WARNING(L"The file " << path << L" is not in a package known by Storm. Using a dummy package.");
				package = new (this) Package(new (this) Str(L"dummy package"));
			}

			PkgReader *pkgReader = createReader(new (this) Array<Url *>(1, path), package);
			FileReader *reader = null;
			if (pkgReader)
				reader = pkgReader->readFile(path, src);

			Nat offset = 0;
			while (reader) {
				FileReader *next = reader->next(qParser);
				Part *part = new (e) Part(this, offset, reader, next);
				offset = part->full().to;
				reader = next;
				parts->push(part);
			}
		}

		Range File::full() const {
			Range result;

			if (parts->any()) {
				result = parts->last()->full();
				result.from = 0;
			}

			return result;
		}

		Range File::replace(Range range, Str *replace) {
			// Total invalidated range.
			Range result;
			// PLN(TO_S(this, L"Replace " << range << L" with " << replace->escape()));

			// The first part which needs to be invalidated eventually.
			Nat invalidateFrom = parts->count();
			Nat offset = 0;
			for (Nat i = 0; i < parts->count(); i++) {
				Part *part = parts->at(i);
				part->offset(offset);
				Range full = part->full();
				offset = full.to;

				Bool here = false;
				here |= range.intersects(full);
				// Note: 'range' can be empty, which causes problems when it lies right between two parts.
				// If 'range' is empty and lies between two parts, pick the earliest possible part.
				here |= range.empty() && range.from == full.from;
				// here |= range.empty() && range.from == full.to;

				if (here) {
					// Make sure to 'clip' the range so that it fits inside 'full'.
					Range here(max(full.from, range.from),
							min(full.to, range.to));

					// PLN(TO_S(this, L"Updating " << i << L" " << here));
					result = merge(result, part->replace(here, replace));
					invalidateFrom = min(invalidateFrom, i + 1);

					// Alter 'offset' and 'range' to reflect the fact that the length of this part has probably changed.
					offset = part->full().to;
					if (range.to >= full.to)
						range.to = offset + range.to - full.to;

					// If something was inserted here, make sure not to insert it in future parts.
					replace = emptyStr;
				}
			}

			// At the end of the last part?
			if (parts->any()) {
				Part *last = parts->last();
				if (last->full().to == range.from)
					result = merge(result, last->replace(range, replace));
			}

			// Shall we delay re-parsing of some parts?
			if (invalidateFrom < parts->count()) {
				work->post(new (this) InvalidatePart(this, invalidateFrom, false));
			}

			return result;
		}

		Array<ColoredRange> *File::colors(Range r) {
			Array<ColoredRange> *result = null;

			for (Nat i = 0; i < parts->count(); i++) {
				Part *part = parts->at(i);
				if (!r.intersects(part->full()))
					continue;

				Array<ColoredRange> *n = part->colors(r);
				if (result)
					result->append(n);
				else
					result = n;
			}

			if (!result)
				result = new (this) Array<ColoredRange>();

			return result;
		}

		syntax::TextIndent File::indent(Nat pos) {
			for (Nat i = 0; i < parts->count(); i++) {
				Part *part = parts->at(i);
				if (!part->full().contains(pos))
					continue;

				return part->indent(pos);
			}

			return TextIndent();
		}

		void File::debugOutput(TextOutput *to, Bool tree) const {
			to->writeLine(TO_S(this, L"Content of " << id << L":"));

			Nat size = 0;
			for (Nat i = 0; i < parts->count(); i++) {
				to->writeLine(TO_S(this, L"Range: " << parts->at(i)->full()));
				parts->at(i)->debugOutput(to, tree);
				to->writeLine(new (this) Str(L"--------"));
				size += parts->at(i)->dbg_size();
			}

			to->writeLine(TO_S(this, L"Syntax tree size: " << (size / 1024) << L" kB"));
		}

		// Extract the text from the current state of all parts.
		static Str *content(Array<Part *> *parts) {
			StrBuf *out = new (parts) StrBuf();
			for (Nat i = 0; i < parts->count(); i++)
				parts->at(i)->text(out);
			return out->toS();
		}

		void File::findError() {
			Str *src = content(parts);
			PkgReader *pkgReader = createReader(new (this) Array<Url *>(1, path), package);
			if (!pkgReader)
				return;
			FileReader *reader = pkgReader->readFile(path, src);

			while (reader) {
				FileReader *next = reader->next(qParser);

				Str::Iter end = src->end();
				if (next)
					end = next->info->start;

				InfoParser *parser = reader->createParser();
				parser->parse(src, path, reader->info->start);
				if (!parser->hasTree())
					throw parser->error();
				if (parser->matchEnd() != end)
					throw parser->error();

				reader = next;
			}
		}

		Range File::updatePart(Nat part, Bool force) {
			try {
				Range result;
				Str *src = content(parts);

				PkgReader *pkgReader = createReader(new (this) Array<Url *>(1, path), package);
				FileReader *reader = null;
				if (pkgReader)
					reader = pkgReader->readFile(path, src);

				Array<Part *> *newParts = new (this) Array<Part *>();
				while (reader) {
					FileReader *next = reader->next(qParser);
					Nat id = newParts->count();

					Nat offset = newParts->any() ? newParts->last()->full().to : 0;
					if (id < part) {
						Part *old = parts->at(id);
						old->adjustOffset(offset, src);
						newParts->push(old);
					} else if (id < parts->count()) {
						Part *part = parts->at(id);
						part->adjustOffset(offset, src);
						if (part->replace(reader, next, force)) {
							result = merge(result, part->full());
						}
						newParts->push(part);
					} else {
						Part *part = new (this) Part(this, offset, reader, next);
						newParts->push(part);
						result = merge(result, part->full());
					}

					reader = next;
				}

				parts = newParts;

				return result;
			} catch (const Exception *) {
				// TODO: More narrow catch?
				return Range();
			}
		}

		void File::postInvalidate(Part *part, Int delta, Bool force) {
			for (Nat i = 0; i < parts->count(); i++) {
				if (part == parts->at(i)) {
					Nat r = i;
					if (delta >= 0)
						r += delta;
					else if (Nat(-delta) > r)
						r = 0;
					else
						r += delta;

					work->post(new (this) InvalidatePart(this, r, force));
				}
			}
		}

		void File::postInvalidate(Part *part, Border border, Bool force) {
			for (Nat i = 0; i < parts->count(); i++) {
				if (part == parts->at(i)) {
					switch (border) {
					case prevBorder:
						// Invalidate from the previous part, if there is one.
						if (i == 0)
							work->post(new (this) InvalidatePart(this, 0, force));
						else
							work->post(new (this) InvalidatePart(this, i - 1, force));
						break;
					case nextBorder:
						// If 'part' is the last part, there is no point in invalidating anything.
						if (i + 1 < parts->count())
							work->post(new (this) InvalidatePart(this, i, force));
						break;
					}
					break;
				}
			}
		}


		/**
		 * Work items posted.
		 */

		InvalidatePart::InvalidatePart(File *file, Nat part, Bool force) : WorkItem(file), part(part), force(force) {}

		Range InvalidatePart::run(WorkQueue *q) {
			return file->updatePart(part, force);
		}

		Bool InvalidatePart::merge(WorkItem *other) {
			if (!WorkItem::merge(other))
				return false;

			InvalidatePart *o = (InvalidatePart *)other;
			return part == o->part && force == o->force;
		}

	}
}
