#pragma once
#include "Core/EnginePtr.h"
#include "Point.h"

namespace storm {
	class Str;
	class StrBuf;
	namespace geometry {
		STORM_PKG(core.geometry);

		/**
		 * A point in 3D-space.
		 */
		class Vector {
			STORM_VALUE;
		public:
			// Zero.
			STORM_CTOR Vector();

			// Initialize.
			STORM_CTOR Vector(Float x, Float y, Float z);
			STORM_CAST_CTOR Vector(Point p);

			// Coordinates.
			Float x;
			Float y;
			Float z;

			// Compute the length squared of the vector. Cheaper to compute than the length.
			Float STORM_FN lengthSq() const;

			// Compute the length of the vector.
			Float STORM_FN length() const;

			// Return a normalized vector (i.e. length 1).
			Vector STORM_FN normalized() const;

			// Compute the taxicab/manhattan length (i.e. x + y + z).
			Float STORM_FN taxiLength() const;

			// Output.
			void STORM_FN toS(StrBuf *to) const;
		};

		wostream &operator <<(wostream &to, const Vector &s);

		// Operations.
		Vector STORM_FN operator +(Vector a, Vector b);
		Vector STORM_FN operator -(Vector a, Vector b);
		Vector STORM_FN operator -(Vector a);
		Vector &STORM_FN operator +=(Vector &a, Vector b);
		Vector &STORM_FN operator -=(Vector &a, Vector b);

		// Scaling.
		Vector STORM_FN operator *(Vector a, Float b);
		Vector STORM_FN operator *(Float a, Vector b);
		Vector STORM_FN operator /(Vector a, Float b);
		Vector &STORM_FN operator *=(Vector &a, Float b);
		Vector &STORM_FN operator /=(Vector &a, Float b);

		// Dot product of two vectors.
		Float STORM_FN operator *(Vector a, Vector b);

		// Cross product of two vectors.
		Vector STORM_FN operator /(Vector a, Vector b);

		inline Bool STORM_FN operator ==(Vector a, Vector b) { return a.x == b.x && a.y == b.y && a.z == b.z; }
		inline Bool STORM_FN operator !=(Vector a, Vector b) { return !(a == b); }

		// Compute the absolute value of both compontents in the point.
		Vector STORM_FN abs(Vector a);

		// Project the point `pt` onto a line that starts in `origin` and expands in the direction
		// of `dir`. `dir` does not have to be normalized.
		Vector STORM_FN project(Vector pt, Vector origin, Vector dir);

	}
}
