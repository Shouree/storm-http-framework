#pragma once

namespace storm {
	class Str;
	namespace geometry {
		class Point;
		STORM_PKG(core.geometry);

		/**
		 * Angle in radians. 0 deg is assumed to be upward in screen-space (ie 0, -1)
		 * TODO: Some kind of comparisons for angles.
		 */
		class Angle {
			STORM_VALUE;
		public:
			// Zero angle.
			inline STORM_CTOR Angle() { v = 0; }

			// Angle in radians (intentionally not visible to Storm).
			inline Angle(Float rad) { v = rad; }

			// Return a normalized angle that lies between 0 and 360 degrees.
			Angle STORM_FN normalized() const;

			// Return an angle in the opposite direction. Normalizes the result after the
			// computation.
			Angle STORM_FN opposite() const;

			// Convert to radians.
			inline Float STORM_FN rad() const { return v; }

			// Convert to degrees.
			inline Float STORM_FN deg() const { return float(v * 180.0 / M_PI); }

			// Output.
			void STORM_FN toS(StrBuf *to) const;

		private:
			// Value (intentionally not visible to Storm).
			Float v;
		};

		// Add/subtract.
		Angle STORM_FN operator +(Angle a, Angle b);
		Angle STORM_FN operator -(Angle a, Angle b);
		Angle STORM_FN operator -(Angle a);

		// Scale.
		Angle STORM_FN operator *(Angle a, Float b);
		Angle STORM_FN operator *(Float a, Angle b);
		Angle STORM_FN operator /(Angle a, Float b);


		// To String.
		wostream &operator <<(wostream &to, Angle a);

		// Create an angle in degrees.
		Angle STORM_FN deg(Float v);

		// Create an angle in radians.
		Angle STORM_FN rad(Float v);

		// Compute the sin of an angle.
		Float STORM_FN sin(Angle v);

		// Compute the cos of an angle.
		Float STORM_FN cos(Angle v);

		// Compute the tan of an angle.
		Float STORM_FN tan(Angle v);

		// Compute asin of a float.
		Angle STORM_FN asin(Float v);

		// Compute acos of a float.
		Angle STORM_FN acos(Float v);

		// Compute atan of a float.
		Angle STORM_FN atan(Float v);

		// Compute the angle of the point. Zero degrees is the point (0, -1), i.e. downwards in screen space.
		Angle STORM_FN angle(Point pt);

		// Get pi.
		inline Float STORM_FN pi() { return Float(M_PI); }
	}
}
