#include "stdafx.h"
#include "Vector.h"
#include "Core/Str.h"
#include "Core/StrBuf.h"

namespace storm {
	namespace geometry {

		Vector::Vector() : x(0), y(0), z(0) {}

		Vector::Vector(Float x, Float y, Float z) : x(x), y(y), z(z) {}

		Vector::Vector(Point p) : x(p.x), y(p.y), z(0) {}

		Float Vector::lengthSq() const {
			return *this * *this;
		}

		Float Vector::length() const {
			return sqrt(lengthSq());
		}

		Vector Vector::normalized() const {
			Float len = length();
			if (len == 0) {
				return (*this);
			} else {
				return (*this) / len;
			}
		}

		Float Vector::taxiLength() const {
			return ::fabs(x) + ::fabs(y) + ::fabs(z);
		}

		wostream &operator <<(wostream &to, const Vector &s) {
			return to << L"(" << s.x << L", " << s.y << L", " << s.z << L")";
		}

		void Vector::toS(StrBuf *to) const {
			*to << S("(") << x << S(", ") << y << S(", ") << z << S(")");
		}

		Vector operator +(Vector a, Vector b) {
			return Vector(a.x + b.x, a.y + b.y, a.z + b.z);
		}

		Vector operator -(Vector a, Vector b) {
			return Vector(a.x - b.x, a.y - b.y, a.z - b.z);
		}

		Vector operator -(Vector a) {
			return Vector(-a.x, -a.y, -a.z);
		}

		Vector &operator +=(Vector &a, Vector b) {
			a.x += b.x;
			a.y += b.y;
			a.z += b.z;
			return a;
		}

		Vector &operator -=(Vector &a, Vector b) {
			a.x -= b.x;
			a.y -= b.y;
			a.z -= b.z;
			return a;
		}

		Vector operator *(Vector a, Float b) {
			return Vector(a.x * b, a.y * b, a.z * b);
		}

		Vector operator *(Float a, Vector b) {
			return Vector(a * b.x, a * b.y, a * b.z);
		}

		Vector operator /(Vector a, Float b) {
			return Vector(a.x / b, a.y / b, a.z / b);
		}

		Vector &operator *=(Vector &a, Float b) {
			a.x *= b;
			a.y *= b;
			a.z *= b;
			return a;
		}

		Vector &operator /=(Vector &a, Float b) {
			a.x /= b;
			a.y /= b;
			a.z /= b;
			return a;
		}

		// Dot and cross product.
		Float operator *(Vector a, Vector b) {
			return a.x * b.x + a.y * b.y + a.z * b.z;
		}

		Vector operator /(Vector a, Vector b) {
			return Vector(a.y*b.z - a.z*b.y, a.z*b.x - a.x*b.z, a.z*b.y - a.y*b.z);
		}

		Vector abs(Vector a) {
			return Vector(::fabs(a.x), ::fabs(a.y), ::fabs(a.z));
		}

		Vector project(Vector pt, Vector origin, Vector dir) {
			pt -= origin;
			float t = (pt * dir) / (dir * dir);
			return origin + dir * t;
		}

	}
}
