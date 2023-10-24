#pragma once
#include "Size.h"
#include "Angle.h"

namespace storm {
	namespace geometry {
		STORM_PKG(core.geometry);

		/**
		 * A point in 2D-space.
		 * Screen-space is assumed, where 0,0 is positioned in the upper left corner of the screen.
		 */
		class Point {
			STORM_VALUE;
		public:
			// Zero.
			STORM_CTOR Point();

			// Initialize.
			STORM_CTOR Point(Float x, Float y);

			// Convert.
			STORM_CAST_CTOR Point(Size s);

			// Coordinates.
			Float x;
			Float y;

			// Tangent. Always 90 deg clockwise from the vector in screen-space.
			Point STORM_FN tangent() const;

			// Compute the length squared of the point. Cheaper than the length.
			Float STORM_FN lengthSq() const;

			// Compute the lenght of the point.
			Float STORM_FN length() const;

			// Return a normalized point (i.e. length 1).
			Point STORM_FN normalized() const;

			// Compute the taxicab/manhattan length (i.e. x + y).
			Float STORM_FN taxiLength() const;
		};

		wostream &operator <<(wostream &to, const Point &s);
		StrBuf &STORM_FN operator <<(StrBuf &to, Point s);

		// Operations.
		Point STORM_FN operator +(Point a, Size b);
		Point STORM_FN operator +(Point a, Point b);
		Point STORM_FN operator -(Point a, Point b);
		Point STORM_FN operator -(Point a, Size b);
		Point STORM_FN operator -(Point a);
		Point &STORM_FN operator +=(Point &a, Point b);
		Point &STORM_FN operator -=(Point &a, Point b);

		Point STORM_FN operator *(Point a, Float b);
		Point STORM_FN operator *(Float a, Point b);
		Point STORM_FN operator /(Point a, Float b);
		Point &STORM_FN operator *=(Point &a, Float b);
		Point &STORM_FN operator /=(Point &a, Float b);

		// Dot product.
		Float STORM_FN operator *(Point a, Point b);

		inline Bool STORM_FN operator ==(Point a, Point b) { return a.x == b.x && a.y == b.y; }
		inline Bool STORM_FN operator !=(Point a, Point b) { return !(a == b); }

		// Create an angle based on the point. 0 deg is (0, -1)
		Point STORM_FN angle(Angle angle);

		// Project the point `pt` onto a line that starts in `origin` and expands in the direction
		// of `dir`. `dir` does not have to be normalized.
		Point STORM_FN project(Point pt, Point origin, Point dir);

		// Center point of a size.
		Point STORM_FN center(Size s);

		// Compute the absolute value of both compontents in the point.
		Point STORM_FN abs(Point a);

	}
}
