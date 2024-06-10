#include "stdafx.h"
#include "Color.h"
#include "Core/StrBuf.h"

namespace storm {

	static float toF(byte b) {
		return float(b) / 255.0f;
	}

	static inline float clamp(float v) {
		return max(min(v, 1.0f), 0.0f);
	}

	Color::Color() : r(0), g(0), b(0), a(1) {}

	Color::Color(Byte r, Byte g, Byte b) : r(toF(r)), g(toF(g)), b(toF(b)), a(1.0f) {}
	Color::Color(Byte r, Byte g, Byte b, Byte alpha) : r(toF(r)), g(toF(g)), b(toF(b)), a(toF(alpha)) {}

	Color::Color(float r, float g, float b) : r(clamp(r)), g(clamp(g)), b(clamp(b)), a(1.0f) {}
	Color::Color(float r, float g, float b, float a) : r(clamp(r)), g(clamp(g)), b(clamp(b)), a(clamp(a)) {}

	Color Color::operator +(const Color &o) const {
		return Color(r*a + o.r*o.a, g*a + o.g*o.a, b*a + o.b*o.a, a * o.a);
	}

	Color Color::operator -(const Color &o) const {
		if (o.a == 0.0f)
			return *this;
		else
			return Color(r*a - o.r*o.a, g*a - o.g*o.a, b*a - o.b*o.a, a / o.a);
	}

	Color Color::operator *(float f) const {
		return Color(r * f, g * f, b * f, a);
	}

	Color Color::operator /(float f) const {
		return Color(r / f, g / f, b / f, a);
	}

	Color Color::withAlpha(Float a) const {
		return Color(r, g, b, a);
	}

	wostream &operator <<(wostream &to, const Color &s) {
		return to << L"(" << s.r << L", " << s.g << L", " << s.b << L", " << s.a << L")";
	}

	void Color::toS(StrBuf *to) const {
		*to << S("(") << r << S(", ") << g << S(", ") << b << S(", ") << a << S(")");
	}

	Color transparent() { return Color(0.0f, 0.0f, 0.0f, 0.0f); }
	Color black()  { return Color(0.0f, 0.0f, 0.0f); }
	Color white()  { return Color(1.0f, 1.0f, 1.0f); }
	Color red()    { return Color(1.0f, 0.0f, 0.0f); }
	Color green()  { return Color(0.0f, 1.0f, 0.0f); }
	Color blue()   { return Color(0.0f, 0.0f, 1.0f); }
	Color yellow() { return Color(1.0f, 1.0f, 0.0f); }
	Color cyan()   { return Color(0.0f, 1.0f, 1.0f); }
	Color pink()   { return Color(1.0f, 0.0f, 1.0f); }

}
