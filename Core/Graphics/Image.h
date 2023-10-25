#pragma once
#include "Core/Object.h"
#include "Core/Geometry/Point.h"
#include "Core/Geometry/Size.h"
#include "Color.h"

namespace storm {
	STORM_PKG(graphics);

	/**
	 * A 32-bit RGBA image.
	 */
	class Image : public Object {
		STORM_CLASS;
	public:
		// Empty image.
		STORM_CTOR Image();

		// Copy.
		Image(const Image &o);

		// Create an image with the specified size. The image is initially entirely transparent
		// (based on the color black).
		STORM_CTOR Image(geometry::Size size);
		STORM_CTOR Image(Nat w, Nat h);

		// Get the width of the image.
		inline Nat STORM_FN width() { return w; }

		// Get the height of the image.
		inline Nat STORM_FN height() { return h; }

		// Get the size of the image.
		geometry::Size STORM_FN size();

		// Does this image contain any pixels that are transparent or semi-transparent?
		Bool STORM_FN hasAlpha();

		// Get the pixel at a coordinate.
		Color STORM_FN get(Nat x, Nat y);

		// Get the pixel at the specified coordinate. Truncates any fractions, does not perform any interpolation.
		Color STORM_FN get(geometry::Point at);

		// Set the pixel at the specified coordinate.
		void STORM_FN set(Nat x, Nat y, Color c);
		void STORM_FN set(geometry::Point p, Color c);

		// TODO: Also provide image[px] = x

		// Raw buffer information:

		// Stride, difference between each row (in bytes).
		Nat stride() const;

		// Size of buffer.
		Nat bufferSize() const;

		// Raw buffer access.
		byte *buffer();
		byte *buffer(Nat x, Nat y);

	private:
		// Data.
		GcArray<Byte> *data;

		// Size.
		Nat w;
		Nat h;

		// Compute offset of pixel.
		Nat offset(Nat x, Nat y);
	};

}
