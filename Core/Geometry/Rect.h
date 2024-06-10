#pragma once
#include "Size.h"
#include "Point.h"

namespace storm {
	namespace geometry {
		STORM_PKG(core.geometry);

		/**
		 * 2D rectangle represented by two points p0 and p1 (top left, bottom right).
		 */
		class Rect {
			STORM_VALUE;
		public:
			// Create an empty rectangle.
			STORM_CTOR Rect();

			// Initialize to (0, 0)-(size)
			STORM_CTOR Rect(Size s);

			// Initialize to point and size.
			STORM_CTOR Rect(Point p, Size s);

			// Initialize to two points.
			STORM_CTOR Rect(Point topLeft, Point bottomRight);

			// Initialize to raw coordinates.
			STORM_CTOR Rect(Float left, Float top, Float right, Float bottom);

			// Data.
			Point p0;
			Point p1;

			// Return a normalized rectangle, where `p0` is to the top-left of `p1`.
			Rect STORM_FN normalized() const;

			// Does this rectangle contain the point `pt`.
			Bool STORM_FN contains(Point pt) const;

			// Does this rectangle contain another rectangle? For this to be true, all parts of
			// `rect` needs to be inside this rectangle.
			Bool STORM_FN contains(Rect rect) const;

			// Does this rectangle intersect with another rectangle?
			Bool STORM_FN intersects(Rect other) const;

			// Size.
			inline Size STORM_FN size() const { return p1 - p0; }
			inline void STORM_ASSIGN size(Size to) { p1 = p0 + to; }

			// Return a rectangle with the same origin, but the specified size.
			inline Rect STORM_FN sized(Size to) { return Rect(p0, to); }

			// Compute the center of the rectangle.
			inline Point STORM_FN center() const { return (p0 + p1) / 2; }

			// Move the origin of the rectangle to `to`.
			inline Rect STORM_FN at(Point to) const { return Rect(to, size()); }

			Rect STORM_FN operator +(Point pt) const;
			Rect STORM_FN operator -(Point pt) const;
			Rect &STORM_FN operator +=(Point pt);
			Rect &STORM_FN operator -=(Point pt);

			// Increase the rectangle to include the point `to`. Can be seen as a single step in
			// computing a bounding box of a number of points.
			Rect STORM_FN include(Point to) const;

			// Increase the dimensions of the rectangle to include `other`. Can be seen as the union
			// of two bounding boxes.
			Rect STORM_FN include(Rect other) const;

			// Scale the entire rect around its center.
			Rect STORM_FN scaled(Float scale) const;

			// Shrink the rectangle in all dimensions based on the height and width in `rate`.
			Rect STORM_FN shrink(Size rate) const;

			// Grow the rectangle in all dimensions based on the height and width in `rate`.
			Rect STORM_FN grow(Size rate) const;

			// Output.
			void STORM_FN toS(StrBuf *to) const;
		};

		// Check if the point `pt` is inside the rectangle `r`. Same as `r.contains(pt)`.
		Bool STORM_FN inside(Point pt, Rect r);

		// ToS.
		wostream &operator <<(wostream &to, Rect r);

		inline Bool STORM_FN operator ==(Rect a, Rect b) { return a.p0 == b.p0 && a.p1 == b.p1; }
		inline Bool STORM_FN operator !=(Rect a, Rect b) { return !(a == b); }
	}
}
