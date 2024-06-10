#pragma once
#include "Core/EnginePtr.h"

namespace storm {
	class Str;
	class StrBuf;

	namespace geometry {
		class Point;
		STORM_PKG(core.geometry);

		/**
		 * Size in 2D-space. If either 'w' or 'h' is negative, the size is considered invalid.
		 */
		class Size {
			STORM_VALUE;
		public:
			// Create a size of zero.
			STORM_CTOR Size();

			// Create a size with width and height set to the same value.
			STORM_CTOR Size(Float wh);

			// Create a size with separate width and height.
			STORM_CTOR Size(Float w, Float h);

			// Create a size from a point.
			STORM_CAST_CTOR Size(Point pt);

			Float w;
			Float h;

			// Check if the components are non-negative.
			Bool STORM_FN valid() const;

			// Component-wise minimum of two sizes.
			Size STORM_FN min(Size o) const;

			// Component-wise maximum of two sizes.
			Size STORM_FN max(Size o) const;

			// Output.
			void STORM_FN toS(StrBuf *to) const;
		};

		Size STORM_FN operator +(Size a, Size b);
		Size STORM_FN operator -(Size a, Size b);
		Size STORM_FN operator *(Float s, Size a);
		Size STORM_FN operator *(Size a, Float s);
		Size STORM_FN operator /(Size a, Float s);

		inline Bool STORM_FN operator ==(Size a, Size b) { return a.w == b.w && a.h == b.h; }
		inline Bool STORM_FN operator !=(Size a, Size b) { return !(a == b); }

		// Normalize a size by taking its absolute value.
		Size STORM_FN abs(Size a);

		using std::min;
		using std::max;

		// Get the maximum of the two components.
		Float STORM_FN max(Size x);

		// Get the minimum of the two components.
		Float STORM_FN min(Size x);

		wostream &operator <<(wostream &to, const Size &s);
	}
}
