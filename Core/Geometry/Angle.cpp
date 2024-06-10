#include "stdafx.h"
#include "Angle.h"
#include "Str.h"
#include "Point.h"
#include "Core/StrBuf.h"

namespace storm {
	namespace geometry {

		Angle Angle::normalized() const {
			Int sub = Int(floor(v / (2 * M_PI)));
			return Angle(v - Float(sub * (2 * M_PI)));
		}

		Angle Angle::opposite() const {
			return Angle(Float(v + M_PI)).normalized();
		}

		wostream &operator <<(wostream &to, Angle a) {
			return to << a.deg() << L" deg";
		}

		void Angle::toS(StrBuf *to) const {
			*to << deg() << S(" deg");
		}

		Angle deg(Float v) {
			return Angle(float(v * M_PI / 180.0));
		}

		Angle rad(Float v) {
			return Angle(v);
		}

		Angle operator +(Angle a, Angle b) {
			return Angle(a.rad() + b.rad());
		}

		Angle operator -(Angle a, Angle b) {
			return Angle(a.rad() - b.rad());
		}

		Angle operator -(Angle a) {
			return Angle(-a.rad());
		}

		Angle operator *(Angle a, Float b) {
			return Angle(a.rad() * b);
		}

		Angle operator *(Float a, Angle b) {
			return Angle(a * b.rad());
		}

		Angle operator /(Angle a, Float b) {
			return Angle(a.rad() / b);
		}

		Float sin(Angle v) {
			return ::sin(v.rad());
		}

		Float cos(Angle v) {
			return ::cos(v.rad());
		}

		Float tan(Angle v) {
			return ::tan(v.rad());
		}

		Angle asin(Float v) {
			return rad(::asin(v));
		}

		Angle acos(Float v) {
			return rad(::acos(v));
		}

		Angle atan(Float v) {
			return rad(::atan(v));
		}

		Angle angle(Point pt) {
			return rad(::atan2(pt.x, -pt.y));
		}

	}
}
