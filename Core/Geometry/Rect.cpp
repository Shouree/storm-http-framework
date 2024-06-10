#include "stdafx.h"
#include "Rect.h"
#include "Core/Str.h"
#include "Core/StrBuf.h"

namespace storm {
	namespace geometry {

		Rect::Rect() : p0(), p1() {}

		Rect::Rect(Size s) : p0(), p1(s) {}

		Rect::Rect(Point p, Size s) : p0(p), p1(p + s) {}

		Rect::Rect(Point topLeft, Point bottomRight) : p0(topLeft), p1(bottomRight) {}

		Rect::Rect(Float left, Float top, Float right, Float bottom) : p0(left, top), p1(right, bottom) {}

		Rect Rect::normalized() const {
			return Rect(
				Point(min(p0.x, p1.x), min(p0.y, p1.y)),
				Point(max(p0.x, p1.x), max(p0.y, p1.y))
				);
		}

		Bool Rect::contains(Point p) const {
			return (p0.x <= p.x) && (p.x < p1.x)
				&& (p0.y <= p.y) && (p.y < p1.y);
		}

		Bool Rect::contains(Rect r) const {
			return (p0.x <= r.p0.x) && (r.p1.x < p1.x)
				&& (p0.y <= r.p0.y) && (r.p1.y < p1.y);
		}

		Bool inside(Point pt, Rect r) {
			return r.contains(pt);
		}

		Bool Rect::intersects(Rect other) const {
			return (p0.x < other.p1.x) && (other.p0.x < p1.x)
				&& (p0.y < other.p1.y) && (other.p0.y < p1.y);
		}

		Rect Rect::operator +(Point pt) const {
			return Rect(p0 + pt, p1 + pt);
		}

		Rect Rect::operator -(Point pt) const {
			return Rect(p0 - pt, p1 - pt);
		}

		Rect &Rect::operator +=(Point pt) {
			p0 += pt;
			p1 += pt;
			return *this;
		}

		Rect &Rect::operator -=(Point pt) {
			p0 -= pt;
			p1 -= pt;
			return *this;
		}

		Rect Rect::include(Point to) const {
			Float l = min(p0.x, to.x);
			Float t = min(p0.y, to.y);
			Float r = max(p1.x, to.x);
			Float b = max(p1.y, to.y);
			return Rect(l, t, r, b);
		}

		Rect Rect::include(Rect other) const {
			return include(other.p0).include(other.p1);
		}

		Rect Rect::scaled(Float scale) const {
			Size s = size() / 2;
			Float dx = s.w * scale - s.w;
			Float dy = s.h * scale - s.h;

			Float l = p0.x - dx;
			Float t = p0.y - dy;
			Float r = p1.x + dx;
			Float b = p1.y + dy;
			return Rect(l, t, r, b);
		}

		Rect Rect::shrink(Size s) const {
			Float l = p0.x + s.w;
			Float t = p0.y + s.h;
			Float r = p1.x - s.w;
			Float b = p1.y - s.h;
			return Rect(l, t, r, b);
		}

		Rect Rect::grow(Size s) const {
			Float l = p0.x - s.w;
			Float t = p0.y - s.h;
			Float r = p1.x + s.w;
			Float b = p1.y + s.h;
			return Rect(l, t, r, b);
		}

		wostream &operator <<(wostream &to, Rect r) {
			return to << r.p0 << L"-" << r.p1;
		}

		void Rect::toS(StrBuf *to) const {
			*to << p0 << S("-") << p1;
		}

	}
}
