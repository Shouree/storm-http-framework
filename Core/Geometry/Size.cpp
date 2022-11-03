#include "stdafx.h"
#include "Size.h"
#include "Point.h"
#include "Core/Str.h"
#include "Core/StrBuf.h"

namespace storm {
	namespace geometry {

		Size::Size() : w(0), h(0) {}

		Size::Size(Float wh) : w(wh), h(wh) {}

		Size::Size(Float w, Float h) : w(w), h(h) {}

		Size::Size(Point pt) : w(pt.x), h(pt.y) {}

		Bool Size::valid() const {
			return w >= 0.0f && h >= 0.0f;
		}

		wostream &operator <<(wostream &to, const Size &s) {
			return to << L"(" << s.w << L", " << s.h << L")";
		}

		StrBuf &operator <<(StrBuf &to, Size s) {
			return to << L"(" << s.w << L", " << s.h << L")";
		}

		Size STORM_FN operator +(Size a, Size b) {
			return Size(a.w + b.w, a.h + b.h);
		}

		Size STORM_FN operator -(Size a, Size b) {
			return Size(a.w - b.w, a.h - b.h);
		}

		Size STORM_FN operator *(Float s, Size a) {
			return Size(a.w * s, a.h * s);
		}

		Size STORM_FN operator *(Size a, Float s) {
			return Size(a.w * s, a.h * s);
		}

		Size STORM_FN operator /(Size a, Float s) {
			return Size(a.w / s, a.h / s);
		}

		Size STORM_FN Size::min(Size o) const {
			return Size(::min(w, o.w), ::min(h, o.h));
		}

		Size STORM_FN Size::max(Size o) const {
			return Size(::max(w, o.w), ::max(h, o.h));
		}

		Size STORM_FN abs(Size a) {
			return Size(::fabs(a.w), ::fabs(a.h));
		}

		Float STORM_FN max(Size x) {
			return max(x.w, x.h);
		}

		Float STORM_FN min(Size x) {
			return min(x.w, x.h);
		}

	}
}
