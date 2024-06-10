#pragma once

namespace storm {
	class StrBuf;

	STORM_PKG(graphics);

	/**
	 * Color + alpha channel. Alpha goes from 0 (transparent) to 1 (opaque)
	 */
	class Color {
		STORM_VALUE;
	public:
		// Create black (default).
		STORM_CTOR Color();

		// Create from 8-bit bytes.
		STORM_CTOR Color(Byte r, Byte g, Byte b);
		STORM_CTOR Color(Byte r, Byte g, Byte b, Byte alpha);

		// Create from Floats.
		STORM_CTOR Color(Float r, Float g, Float b);
		STORM_CTOR Color(Float r, Float g, Float b, Float a);

		// Actual values, in the range [0-1]
		Float r;
		Float g;
		Float b;
		Float a;

		// Calculations. Adding colors result in their alpha channels being multiplied. TODO: How should - act?
		Color STORM_FN operator +(const Color &b) const;
		Color STORM_FN operator -(const Color &b) const;

		// These do not alter the alpha channel.
		Color STORM_FN operator *(Float factor) const;
		Color STORM_FN operator /(Float factor) const;

		// Get a variant with a different alpha value.
		Color STORM_FN withAlpha(Float f) const;

		// Output.
		void STORM_FN toS(StrBuf *to) const;
	};

	wostream &operator <<(wostream &to, const Color &s);

	// Some default colors.
	Color STORM_FN transparent(); // Based on black.
	Color STORM_FN black();
	Color STORM_FN white();
	Color STORM_FN red();
	Color STORM_FN green();
	Color STORM_FN blue();
	Color STORM_FN yellow();
	Color STORM_FN cyan();
	Color STORM_FN pink();

}
