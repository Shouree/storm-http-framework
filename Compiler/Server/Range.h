#pragma once
#include "Core/StrBuf.h"
#include "Compiler/Syntax/TokenColor.h"

namespace storm {
	namespace server {
		STORM_PKG(core.lang.server);

		/**
		 * Represents a range in a file.
		 */
		class Range {
			STORM_VALUE;
		public:
			STORM_CTOR Range();
			STORM_CTOR Range(Nat from, Nat to);

			Nat from;
			Nat to;

			// Is this range empty?
			inline Bool STORM_FN empty() const { return from == to; }

			// Does this range intersect another range?
			Bool STORM_FN intersects(Range other) const;

			// Does this range contain another point?
			Bool STORM_FN contains(Nat pt) const;

			// Distance from this range to a point.
			Nat STORM_FN distance(Nat pt) const;

			// Size of this range.
			inline Nat STORM_FN count() const { return to - from; }

			// Deep copy.
			void STORM_FN deepCopy(CloneEnv *env);

			// Equality.
			inline Bool STORM_FN operator ==(const Range &o) const {
				return to == o.to && from == o.from;
			}
			inline Bool STORM_FN operator !=(const Range &o) const {
				return !(*this == o);
			}

			// Output.
			void STORM_FN toS(StrBuf *to) const;
		};

		wostream &operator <<(wostream &to, Range r);

		// Compute the union of two ranges.
		Range STORM_FN merge(Range a, Range b);

		/**
		 * Represents a a region to be colored in a file.
		 */
		class ColoredRange {
			STORM_VALUE;
		public:
			STORM_CTOR ColoredRange(Range r, syntax::TokenColor c);

			Range range;

			// TODO: Use strings or symbols here?
			syntax::TokenColor color;

			// Output.
			void STORM_FN toS(StrBuf *to) const;
		};

	}
}
