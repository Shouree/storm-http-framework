#include "stdafx.h"
#include "Transform.h"
#include "Core/StrBuf.h"
#include <iomanip>

namespace storm {
	namespace geometry {

		Transform::Transform() {
			for (nat r = 0; r < 4; r++)
				for (nat c = 0; c < 4; c++)
					v(r, c) = (r == c) ? 1.0f : 0.0f;
		}

		Transform::Transform(const Transform &o) {
			memcpy(&v00, &o.v00, sizeof(v00)*4*4);
		}

		Transform::Transform(Float src[16]) {
			nat i = 0;
			for (nat r = 0; r < 4; r++)
				for (nat c = 0; c < 4; c++)
					v(r, c) = src[i++];
		}

		Transform::Transform(Float src[4][4]) {
			for (nat i = 0; i < 4; i++)
				for (nat j = 0; j < 4; j++)
					v(i, j) = src[i][j];
		}

		void Transform::toS(StrBuf *w) const {
			StrFmt old = w->format();
			*w << fixed(2);

			for (Nat r = 0; r < 4; r++) {
				*w << S("\n(");
				for (Nat c = 0; c < 4; c++) {
					if (c != 0)
						*w << S(" ");
					*w << right(7) << at(r, c);
				}
				*w << S(")");
			}

			w->format(old);
		}

		Transform *Transform::operator *(Transform *o) {
			float r[4][4];
			for (nat i = 0; i < 4; i++) {
				for (nat j = 0; j < 4; j++) {
					float sum = 0;
					for (int k = 0; k < 4; k++)
						sum += at(i, k) * o->at(k, j);
					r[i][j] = sum;
				}
			}
			return new (this) Transform(r);
		}

		Vector operator *(Vector o, Transform *tfm) {
			Float x = tfm->v00*o.x + tfm->v10*o.y + tfm->v20*o.z + tfm->v30;
			Float y = tfm->v01*o.x + tfm->v11*o.y + tfm->v21*o.z + tfm->v31;
			Float z = tfm->v02*o.x + tfm->v12*o.y + tfm->v22*o.z + tfm->v32;
			Float w = tfm->v03*o.x + tfm->v13*o.y + tfm->v23*o.z + tfm->v33;
			return Vector(x/w, y/w, z/w);
		}

		Point operator *(Point o, Transform *tfm) {
			float x = tfm->v00*o.x + tfm->v10*o.y + tfm->v30;
			float y = tfm->v01*o.x + tfm->v11*o.y + tfm->v31;
			float w = tfm->v03*o.x + tfm->v13*o.y + tfm->v33;
			return Point(x/w, y/w);
		}

		Transform *Transform::inverted() {
			Float inv[4][4];

			inv[0][0] = v11 * v22 * v33 -
				v11 * v32 * v23 -
				v12 * v21 * v33 +
				v12 * v31 * v23 +
				v13 * v21 * v32 -
				v13 * v31 * v22;

			inv[0][1] = -v01 * v22 * v33 +
				v01 * v32 * v23 +
				v02 * v21 * v33 -
				v02 * v31 * v23 -
				v03 * v21 * v32 +
				v03 * v31 * v22;

			inv[0][2] = v01 * v12 * v33 -
				v01 * v32 * v13 -
				v02 * v11 * v33 +
				v02 * v31 * v13 +
				v03 * v11 * v32 -
				v03 * v31 * v12;

			inv[0][3] = -v01 * v12 * v23 +
				v01 * v22 * v13 +
				v02 * v11 * v23 -
				v02 * v21 * v13 -
				v03 * v11 * v22 +
				v03 * v21 * v12;

			inv[1][0] = -v10 * v22 * v33 +
				v10 * v32 * v23 +
				v12 * v20 * v33 -
				v12 * v30 * v23 -
				v13 * v20 * v32 +
				v13 * v30 * v22;

			inv[1][1] = v00 * v22 * v33 -
				v00 * v32 * v23 -
				v02 * v20 * v33 +
				v02 * v30 * v23 +
				v03 * v20 * v32 -
				v03 * v30 * v22;

			inv[1][2] = -v00 * v12 * v33 +
				v00 * v32 * v13 +
				v02 * v10 * v33 -
				v02 * v30 * v13 -
				v03 * v10 * v32 +
				v03 * v30 * v12;

			inv[1][3] = v00 * v12 * v23 -
				v00 * v22 * v13 -
				v02 * v10 * v23 +
				v02 * v20 * v13 +
				v03 * v10 * v22 -
				v03 * v20 * v12;

			inv[2][0] = v10 * v21 * v33 -
				v10 * v31 * v23 -
				v11 * v20 * v33 +
				v11 * v30 * v23 +
				v13 * v20 * v31 -
				v13 * v30 * v21;

			inv[2][1] = -v00 * v21 * v33 +
				v00 * v31 * v23 +
				v01 * v20 * v33 -
				v01 * v30 * v23 -
				v03 * v20 * v31 +
				v03 * v30 * v21;

			inv[2][2] = v00 * v11 * v33 -
				v00 * v31 * v13 -
				v01 * v10 * v33 +
				v01 * v30 * v13 +
				v03 * v10 * v31 -
				v03 * v30 * v11;

			inv[2][3] = -v00 * v11 * v23 +
				v00 * v21 * v13 +
				v01 * v10 * v23 -
				v01 * v20 * v13 -
				v03 * v10 * v21 +
				v03 * v20 * v11;

			inv[3][0] = -v10 * v21 * v32 +
				v10 * v31 * v22 +
				v11 * v20 * v32 -
				v11 * v30 * v22 -
				v12 * v20 * v31 +
				v12 * v30 * v21;

			inv[3][1] = v00 * v21 * v32 -
				v00 * v31 * v22 -
				v01 * v20 * v32 +
				v01 * v30 * v22 +
				v02 * v20 * v31 -
				v02 * v30 * v21;

			inv[3][2] = -v00 * v11 * v32 +
				v00 * v31 * v12 +
				v01 * v10 * v32 -
				v01 * v30 * v12 -
				v02 * v10 * v31 +
				v02 * v30 * v11;

			inv[3][3] = v00 * v11 * v22 -
				v00 * v21 * v12 -
				v01 * v10 * v22 +
				v01 * v20 * v12 +
				v02 * v10 * v21 -
				v02 * v20 * v11;

			float det = v00 * inv[0][0] + v10 * inv[0][1] + v20 * inv[0][2] + v30 * inv[0][3];

			if (det == 0) {
				assert(false, L"Non-invertible matrix found!");
				return CREATE(Transform, this);
			}

			det = 1.0f / det;

			for (nat i = 0; i < 4; i++)
				for (nat j = 0; j < 4; j++)
					inv[i][j] *= det;

			return CREATE(Transform, this, inv);
		}

		Transform *translate(EnginePtr e, Vector v) {
			float d[] = {
				1,   0,   0,   0,
				0,   1,   0,   0,
				0,   0,   1,   0,
				v.x, v.y, v.z, 1,
			};
			return CREATE(Transform, e.v, d);
		}

		Transform *translate(EnginePtr e, Point v) {
			return translate(e, Vector(v));
		}

		Transform *translate(EnginePtr e, Size v) {
			return translate(e, Point(v));
		}

		Transform *rotateX(EnginePtr e, Angle angle) {
			float s = sin(angle.rad()), c = cos(angle.rad());
			float d[] = {
				1, 0, 0, 0,
				0, c, s, 0,
				0, -s, c, 0,
				0, 0, 0, 1,
			};
			return CREATE(Transform, e.v, d);
		}

		Transform *rotateX(EnginePtr e, Angle angle, Vector origin) {
			float s = sin(angle.rad()), c = cos(angle.rad());
			float d[] = {
				1, 0, 0, 0,
				0, c, s, 0,
				0, -s, c, 0,
				0, -origin.y*c + origin.z*s + origin.y, -origin.y*s - origin.z*c + origin.z, 1,
			};
			return CREATE(Transform, e.v, d);
		}

		Transform *rotateY(EnginePtr e, Angle angle) {
			float s = sin(angle.rad()), c = cos(angle.rad());
			float d[] = {
				c, 0, -s, 0,
				0, 1, 0, 0,
				s, 0, c, 0,
				0, 0, 0, 1,
			};
			return CREATE(Transform, e.v, d);
		}

		Transform *rotateY(EnginePtr e, Angle angle, Vector origin) {
			float s = sin(angle.rad()), c = cos(angle.rad());
			float d[] = {
				c, 0, -s, 0,
				0, 1, 0, 0,
				s, 0, c, 0,
				-origin.x*c - origin.z*s + origin.x, 0, origin.x*s - origin.z*c + origin.z, 1,
			};
			return CREATE(Transform, e.v, d);
		}

		Transform *rotateZ(EnginePtr e, Angle angle) {
			float s = sin(angle.rad()), c = cos(angle.rad());
			float d[] = {
				c, s, 0, 0,
				-s, c, 0, 0,
				0, 0, 1, 0,
				0, 0, 0, 1,
			};
			return CREATE(Transform, e.v, d);
		}

		Transform *rotateZ(EnginePtr e, Angle angle, Vector origin) {
			float s = sin(angle.rad()), c = cos(angle.rad());
			float d[] = {
				c, s, 0, 0,
				-s, c, 0, 0,
				0, 0, 1, 0,
				-origin.x*c + origin.y*s + origin.x, -origin.x*s - origin.y*c + origin.y, 0, 1,
			};
			return CREATE(Transform, e.v, d);
		}

		Transform *rotate(EnginePtr e, Angle angle) {
			return rotateZ(e, angle);
		}

		Transform *rotate(EnginePtr e, Angle angle, Point origin) {
			return rotateZ(e, angle, origin);
		}

		Transform *scale(EnginePtr e, Float scale) {
			float d[] = {
				scale, 0, 0, 0,
				0, scale, 0, 0,
				0, 0, scale, 0,
				0, 0, 0, 1,
			};
			return CREATE(Transform, e.v, d);
		}

		Transform *scale(EnginePtr e, Vector scale) {
			float d[] = {
				scale.x, 0, 0, 0,
				0, scale.y, 0, 0,
				0, 0, scale.z, 0,
				0, 0, 0, 1,
			};
			return CREATE(Transform, e.v, d);
		}

		Transform *scale(EnginePtr e, Float scale, Vector center) {
			float d[] = {
				scale, 0, 0, 0,
				0, scale, 0, 0,
				0, 0, scale, 0,
				center.x - scale*center.x, center.y - scale*center.y, center.z - scale*center.z, 1,
			};
			return CREATE(Transform, e.v, d);
		}

		Transform *scale(EnginePtr e, Vector scale, Vector center) {
			float d[] = {
				scale.x, 0, 0, 0,
				0, scale.y, 0, 0,
				0, 0, scale.z, 0,
				center.x - scale.x*center.x, center.y - scale.y*center.y, center.z - scale.z*center.z, 1,
			};
			return CREATE(Transform, e.v, d);
		}

		Transform *scale(EnginePtr e, Size s) {
			return scale(e, Vector(s.w, s.h, 1.0f));
		}

		Transform *scale(EnginePtr e, Size s, Point p) {
			return scale(e, Vector(s.w, s.h, 1.0f), Vector(p.x, p.y, 0.0f));
		}

		Transform *skewX(EnginePtr e, Angle angle) {
			float t = -tan(angle.rad());
			float d[] = {
				1, 0, 0, 0,
				t, 1, 0, 0,
				t, 0, 1, 0,
				0, 0, 0, 1,
			};
			return CREATE(Transform, e.v, d);
		}

		Transform *skewY(EnginePtr e, Angle angle) {
			float t = -tan(angle.rad());
			float d[] = {
				1, t, 0, 0,
				0, 1, 0, 0,
				0, t, 1, 0,
				0, 0, 0, 1,
			};
			return CREATE(Transform, e.v, d);
		}

		Transform *skewZ(EnginePtr e, Angle angle) {
			float t = -tan(angle.rad());
			float d[] = {
				1, 0, t, 0,
				0, 1, t, 0,
				0, 0, 1, 0,
				0, 0, 0, 1,
			};
			return CREATE(Transform, e.v, d);
		}

	}
}
