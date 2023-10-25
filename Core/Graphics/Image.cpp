#include "stdafx.h"
#include "Image.h"

namespace storm {
	using namespace geometry;

	inline Color toColor(byte *src) {
		return Color(src[0], src[1], src[2], src[3]);
	}

	inline byte toByte(float f) {
		return byte(f * 255.0f);
	}

	inline void fromColor(const Color &src, byte *dest) {
		dest[0] = toByte(src.r);
		dest[1] = toByte(src.g);
		dest[2] = toByte(src.b);
		dest[3] = toByte(src.a);
	}

	Image::Image() : data(null), w(0), h(0) {}

	Image::Image(const Image &o) : data(null), w(o.w), h(o.h) {
		Nat s = w * h * 4;
		data = runtime::allocBuffer(engine(), s);
		memcpy(data->v, o.data->v, s);
	}

	Image::Image(Size size) : data(null), w(Nat(size.w)), h(Nat(size.h)) {
		Nat s = w * h * 4;
		data = runtime::allocBuffer(engine(), s);
	}

	Image::Image(Nat w, Nat h) : data(null), w(w), h(h) {
		Nat s = w * h * 4;
		data = runtime::allocBuffer(engine(), s);
	}

	geometry::Size Image::size() {
		return geometry::Size(Float(w), Float(h));
	}

	Nat Image::offset(Nat x, Nat y) {
		return (y * w + x) * 4;
	}

	Color Image::get(Nat x, Nat y) {
		if (x >= w || y >= h)
			return Color();
		return toColor(data->v + offset(x, y));
	}

	Color Image::get(Point p) {
		// TODO? Interpolate colors?
		return get(Nat(p.x), Nat(p.y));
	}

	void Image::set(Nat x, Nat y, Color c) {
		if (x >= w || y >= h)
			return;
		fromColor(c, data->v + offset(x, y));
	}

	void Image::set(Point p, Color c) {
		set(Nat(p.x), Nat(p.y), c);
	}

	Bool Image::hasAlpha() {
		for (Nat y = 0; y < h; y++) {
			for (Nat x = 0; x < w; x++) {
				byte *b = buffer(x, y);
				if (b[3] != 255)
					return true;
			}
		}

		return false;
	}

	Nat Image::stride() const {
		return w * 4;
	}

	Nat Image::bufferSize() const {
		return w * h * 4;
	}

	byte *Image::buffer() {
		return data->v;
	}

	byte *Image::buffer(Nat x, Nat y) {
		return data->v + offset(x, y);
	}

}
