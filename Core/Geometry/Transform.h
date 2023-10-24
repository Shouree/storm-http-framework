#pragma once
#include "Core/Object.h"
#include "Core/EnginePtr.h"
#include "Vector.h"
#include "Angle.h"

namespace storm {
	namespace geometry {
		STORM_PKG(core.geometry);

		/**
		 * Transform in 2 or 3 dimensions. 2d-coordinates are assumed to lie in the xy-plane.
		 *
		 * We assume that points on the screen take the form of a row vector that is multiplied with
		 * the matrix from the left. Transforms may therefore be combined by multiplying them from
		 * left to right as follows:
		 *
		 * a * b * c
		 *
		 * Which means that a is applied first, then b and finally c.
		 *
		 * Note: we generally view transformations as being applied on the geometry. I.e. that we
		 * transform from world space to screen space.
		 */
		class Transform : public Object {
			STORM_CLASS;
		public:
			// Unit transform.
			STORM_CTOR Transform();

			// Copy.
			Transform(const Transform &o);

			// Create and initialize. Row by row.
			typedef Float FData[16];
			Transform(FData data);

			// Multiply two transforms, creates a transform that performs both original transforms
			// in sequence.
			Transform *STORM_FN operator *(Transform *o);

			// Invert the transform.
			Transform *STORM_FN inverted();

			// Get a single element.
			inline Float STORM_FN at(Nat row, Nat col) const { return (&v00)[row + 4*col]; }

			// To string.
			virtual void STORM_FN toS(StrBuf *to) const;

		private:
			typedef Float FArray[4][4];
			Transform(FArray data);

			inline Float &v(Nat row, Nat col) {
				return (&v00)[row + 4*col];
			}

			// Data (y, x) (row, col). In memory, this is row-by-row.
			Float v00;
			Float v10;
			Float v20;
			Float v30;
			Float v01;
			Float v11;
			Float v21;
			Float v31;
			Float v02;
			Float v12;
			Float v22;
			Float v32;
			Float v03;
			Float v13;
			Float v23;
			Float v33;

			friend Vector operator *(Vector o, Transform *tfm);
			friend Point operator *(Point o, Transform *tfm);
		};

		// Transform a vector using a transform.
		Vector STORM_FN operator *(Vector o, Transform *tfm);

		// Transform a point using a transform.
		Point STORM_FN operator *(Point o, Transform *tfm);


		// Translation.
		Transform *STORM_FN translate(EnginePtr e, Vector v);
		Transform *STORM_FN translate(EnginePtr e, Point v);
		Transform *STORM_FN translate(EnginePtr e, Size v);

		// Rotation.
		Transform *STORM_FN rotateX(EnginePtr e, Angle angle);
		Transform *STORM_FN rotateY(EnginePtr e, Angle angle);
		Transform *STORM_FN rotateZ(EnginePtr e, Angle angle);

		// Rotate around the specified point.
		Transform *STORM_FN rotateX(EnginePtr e, Angle angle, Vector origin);
		Transform *STORM_FN rotateY(EnginePtr e, Angle angle, Vector origin);
		Transform *STORM_FN rotateZ(EnginePtr e, Angle angle, Vector origin);

		// Rotation around the Z axis (in 2D).
		Transform *STORM_FN rotate(EnginePtr e, Angle angle);
		Transform *STORM_FN rotate(EnginePtr e, Angle angle, Point origin);

		// Scale.
		Transform *STORM_FN scale(EnginePtr e, Float scale);
		Transform *STORM_FN scale(EnginePtr e, Vector scale);
		Transform *STORM_FN scale(EnginePtr e, Size scale);

		// Scale, keeping the point 'origin' unchanged (i.e. scaling around 'origin').
		Transform *STORM_FN scale(EnginePtr e, Size scale, Point origin);
		Transform *STORM_FN scale(EnginePtr e, Float scale, Vector origin);
		Transform *STORM_FN scale(EnginePtr e, Vector scale, Vector origin);

		// Skew.
		Transform *STORM_FN skewX(EnginePtr e, Angle angle);
		Transform *STORM_FN skewY(EnginePtr e, Angle angle);
		Transform *STORM_FN skewZ(EnginePtr e, Angle angle);

	}
}
