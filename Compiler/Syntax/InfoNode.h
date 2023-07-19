#pragma once
#include "Core/Str.h"
#include "Compiler/Thread.h"
#include "TokenColor.h"
#include "Token.h"
#include "InfoIndent.h"

namespace storm {
	namespace syntax {
		STORM_PKG(lang.bnf);

		class Production;
		class InfoInternal;
		class InfoLeaf;

		/**
		 * Syntax nodes containing all information about a parse. The main difference from 'Node' is
		 * that this representation contains all nodes, where 'Node' only contain the nodes the
		 * author of the syntax deemed interesting for generating further intermediate
		 * representations. This representation is also able to represent partial parses.
		 *
		 * A partial match is represented as usual, but the partial match may contain string nodes
		 * where an internal node would otherwise be expected. This represents failure to match a
		 * non-terminal.
		 *
		 * This representation is mainly intended for operations like syntax highlighting and
		 * indentation. It is also possible to incrementally update this representation when changes
		 * have been made to the source string since the nodes do not contain any absolute offsets
		 * into the original string.
		 *
		 * TODO: Skip ObjectOn<Compiler> to save some memory.
		 */
		class InfoNode : public ObjectOn<Compiler> {
			STORM_CLASS;
		public:
			// Create.
			STORM_CTOR InfoNode();

			// Specified color of this node.
			TokenColor color;

			// Length of this match (in codepoints).
			Nat STORM_FN length();

			// Error occured during parsing of this node?
			Bool STORM_FN error() const;
			void STORM_FN error(Bool v);

			// Does this node represent a node captured using a delimiter in the grammar?
			Bool STORM_FN delimiter() const;
			void STORM_FN delimiter(Bool v);

			// Find the first leaf node with a non-zero length at position 'pos' relative to this node.
			virtual MAYBE(InfoLeaf *) STORM_FN leafAt(Nat pos);

			// Find the indentation of characters at offset 'pos'. This returns either an absolute
			// number of indentation levels, or another position which indicates that the
			// indentation should be the same as the indentation on that line.
			virtual TextIndent STORM_FN indentAt(Nat pos);

			// Format this info node into a human-readable representation.
			Str *STORM_FN format() const;

			// Generate the formatted string.
			virtual void STORM_FN format(StrBuf *to) const;

			// Get our parent.
			MAYBE(InfoInternal *) STORM_FN parent() const { return parentNode; }

			// Set parent.
			inline void parent(InfoInternal *n) { parentNode = n; }

			// Get the size of this node.
			virtual Nat STORM_FN dbg_size();

		protected:
			// Compute the length of this node.
			virtual Nat STORM_FN computeLength();

			// Invalidate any pre-computed data. Also invalidate any parent nodes.
			void STORM_FN invalidate();

		private:
			// Parent node.
			InfoInternal *parentNode;

			// Data stored here. Stores:
			// msb: set if an error was corrected within this node (ignoring child nodes).
			// rest: cached length of this node. If set to all ones: the length needs to be re-computed.
			Nat data;

			// Masks for 'data'.
			static const Nat errorMask;
			static const Nat delimMask;
			static const Nat lengthMask;
		};


		/**
		 * Internal node. Contains a fixed number of children.
		 */
		class InfoInternal : public InfoNode {
			STORM_CLASS;
		public:
			// Create and allocate space for a pre-defined number of child nodes. Make sure to
			// initialize all elements in the array before letting Storm access this node!
			InfoInternal(Production *prod, Nat children);

			// Create a copy of another node with a different number of elements.
			InfoInternal(InfoInternal *src, Nat children);

			// Information about indentation.
			MAYBE(InfoIndent *) indent;

			// Get our production.
			inline MAYBE(Production *) STORM_FN production() const { return prod; }

			// Number of children.
			inline Nat STORM_FN count() const {
				return Nat(children->count);
			}

			// Get child at offset.
			inline InfoNode *STORM_FN operator [](Nat id) { return at(id); }
			InfoNode *at(Nat id) {
				if (id < count()) {
					return children->v[id];
				} else {
					outOfBounds(id);
					return children->v[0];
				}
			}

			// Set child at offset.
			void set(Nat id, InfoNode *node);

			// Find the first leaf node at position 'pos' relative to this node.
			virtual MAYBE(InfoLeaf *) STORM_FN leafAt(Nat pos);

			// Find the indentation for 'pos'.
			virtual TextIndent STORM_FN indentAt(Nat pos);

			// To string.
			virtual void STORM_FN toS(StrBuf *to) const;

			// Format.
			using InfoNode::format;
			virtual void STORM_FN format(StrBuf *to) const;

			// Get the size of this node.
			virtual Nat STORM_FN dbg_size();

		protected:
			// Compute the lenght of this node.
			virtual Nat STORM_FN computeLength();

		private:
			// Instance of which production?
			Production *prod;

			// Data.
			GcArray<InfoNode *> *children;

			// Throw out of bounds error.
			void outOfBounds(Nat id);
		};


		/**
		 * Leaf node. Contains the string matched at the leaf.
		 */
		class InfoLeaf : public InfoNode {
			STORM_CLASS;
		public:
			// Create.
			STORM_CTOR InfoLeaf(MAYBE(RegexToken *) regex, Str *match);

			// Find leaf nodes.
			virtual MAYBE(InfoLeaf *) STORM_FN leafAt(Nat pos);

			// Set value.
			void set(Str *v);

			// Get the matching regex.
			inline MAYBE(RegexToken *) STORM_FN matches() const { return regex; }

			// Does the content of this node match the regex in here? Returns 'false' if this node
			// does not contain a regex.
			Bool STORM_FN matchesRegex() const;
			Bool STORM_FN matchesRegex(Str *v) const;

			// To string.
			virtual Str *STORM_FN toS() const;

			// To string.
			virtual void STORM_FN toS(StrBuf *to) const;

			// Format.
			using InfoNode::format;
			virtual void STORM_FN format(StrBuf *to) const;

			// Get the size of this node.
			virtual Nat STORM_FN dbg_size();

		protected:
			// Compute the lenght of this node.
			virtual Nat STORM_FN computeLength();

		private:
			// Which regex did this leaf match?
			MAYBE(RegexToken *) regex;

			// The matched string.
			Str *v;
		};

	}
}
