#include "stdafx.h"
#include "InfoNode.h"
#include "Core/Array.h"
#include "Core/Str.h"
#include "Core/StrBuf.h"
#include "Rule.h"
#include "Production.h"

namespace storm {
	namespace syntax {

		const Nat InfoNode::errorMask  = 0x80000000;
		const Nat InfoNode::delimMask  = 0x40000000;
		const Nat InfoNode::lengthMask = 0x3FFFFFFF;

		InfoNode::InfoNode() {
			parentNode = null;
			color = tNone;
			data = 0;
			invalidate();
		}

		Nat InfoNode::length() {
			if ((data & lengthMask) == lengthMask) {
				data &= ~lengthMask;
				data |= computeLength() & lengthMask;
			}

			return data & lengthMask;
		}

		Bool InfoNode::error() const {
			return (data & errorMask) != 0;
		}

		void InfoNode::error(Bool v) {
			if (v)
				data |= errorMask;
			else
				data &= ~errorMask;
		}

		Bool InfoNode::delimiter() const {
			return (data & delimMask) != 0;
		}

		void InfoNode::delimiter(Bool v) {
			if (v)
				data |= delimMask;
			else
				data &= ~delimMask;
		}

		InfoLeaf *InfoNode::leafAt(Nat pos) {
			return null;
		}

		TextIndent InfoNode::indentAt(Nat pos) {
			return TextIndent();
		}

		Nat InfoNode::computeLength() {
			return 0;
		}

		void InfoNode::invalidate() {
			data |= lengthMask;
			if (parentNode)
				parentNode->invalidate();
		}

		Str *InfoNode::format() const {
			StrBuf *to = new (this) StrBuf();
			format(to);
			return to->toS();
		}

		void InfoNode::format(StrBuf *to) const {
			if (delimiter())
				*to << L" (delimiter)";
			if (error())
				*to << L" (contains errors)";
			if (color != tNone)
				*to << L" #" << name(engine(), color);
		}

		Nat InfoNode::dbg_size() {
			return sizeof(InfoNode);
		}

		/**
		 * Internal node.
		 */

		InfoInternal::InfoInternal(Production *p, Nat children) {
			this->prod = p;
			this->children = runtime::allocArray<InfoNode *>(engine(), &pointerArrayType, children);
		}

		InfoInternal::InfoInternal(InfoInternal *src, Nat children) {
			this->prod = src->prod;
			this->children = runtime::allocArray<InfoNode *>(engine(), &pointerArrayType, children);
			Nat copy = min(children, src->count());
			for (Nat i = 0; i < copy; i++)
				this->children->v[i] = src->children->v[i];
		}

		InfoLeaf *InfoInternal::leafAt(Nat pos) {
			for (nat i = 0; i < count(); i++) {
				Nat len = at(i)->length();
				if (pos < len)
					return at(i)->leafAt(pos);
				pos -= len;
			}
			return null;
		}

		TextIndent InfoInternal::indentAt(Nat pos) {
			Nat offset = 0;
			Nat indentStartOffsetBeg = 0;
			Nat indentStartOffsetEnd = 0;

			for (Nat i = 0; i < count(); i++) {
				InfoNode *child = at(i);
				Nat len = child->length();

				if (indent && i + 1 == indent->start)
					indentStartOffsetBeg = offset;
				if (indent && i == indent->start)
					indentStartOffsetEnd = offset;

				if (pos >= offset && pos < offset + len && len > 0) {
					TextIndent r = child->indentAt(pos - offset);
					r.offset(offset);

					if (indent)
						r.applyParent(indent, i, indentStartOffsetBeg, indentStartOffsetEnd);
					return r;
				}

				offset += len;
			}

			return TextIndent();
		}

		Nat InfoInternal::computeLength() {
			Nat len = 0;
			for (Nat i = 0; i < count(); i++)
				len += at(i)->length();
			return len;
		}

		void InfoInternal::outOfBounds(Nat v) {
			throw new (this) ArrayError(v, count());
		}

		void InfoInternal::set(Nat id, InfoNode *node) {
			if (id < count()) {
				children->v[id] = node;
				node->parent(this);
				invalidate();
			} else {
				outOfBounds(id);
			}
		}

		void InfoInternal::toS(StrBuf *to) const {
			for (Nat i = 0; i < children->count; i++)
				children->v[i]->toS(to);
		}

		void InfoInternal::format(StrBuf *to) const {
			*to << L"{";
			{
				Indent z(to);
				if (ProductionType *type = prod->type()) {
					*to << L"\nproduction: " << type->name;
					if (Rule *owner = prod->rule())
						*to << L" of " << owner->name;
				}

				if (indent)
					*to << L"\nindent: " << indent;

				for (Nat i = 0; i < children->count; i++) {
					InfoNode *node = children->v[i];
					*to << L"\n" << node->length() << L" -> ";
					children->v[i]->format(to);
				}
			}
			*to << L"\n}";
			InfoNode::format(to);
		}

		Nat InfoInternal::dbg_size() {
			Nat total = sizeof(InfoInternal);
			total += sizeof(GcArray<InfoNode *>) + (count()*sizeof(InfoNode *)) - sizeof(InfoNode *);

			if (indent)
				total += sizeof(InfoIndent);

			for (Nat i = 0; i < count(); i++) {
				if (children->v[i])
					total += children->v[i]->dbg_size();
			}

			return total;
		}

		/**
		 * Leaf node.
		 */

		InfoLeaf::InfoLeaf(RegexToken *regex, Str *match) : v(match), regex(regex) {}

		InfoLeaf *InfoLeaf::leafAt(Nat pos) {
			return this;
		}

		Nat InfoLeaf::computeLength() {
			// This is in codepoints!
			return v->count();
		}

		void InfoLeaf::set(Str *v) {
			this->v = v;
			invalidate();
		}

		Bool InfoLeaf::matchesRegex() const {
			return matchesRegex(v);
		}

		Bool InfoLeaf::matchesRegex(Str *s) const {
			if (!regex)
				return false;

			return regex->regex.matchAll(s);
		}

		Str *InfoLeaf::toS() const {
			return v;
		}

		void InfoLeaf::toS(StrBuf *to) const {
			*to << v;
		}

		void InfoLeaf::format(StrBuf *to) const {
			*to << L"'" << v->escape('\'') << L"'";
			if (regex)
				*to << L" (matches \"" << regex->regex << L"\")";
			InfoNode::format(to);
		}

		Nat InfoLeaf::dbg_size() {
			// Approximation...
			return sizeof(InfoLeaf) + sizeof(Str) + sizeof(wchar)*length();
		}

	}
}
