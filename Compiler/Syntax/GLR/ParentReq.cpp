#include "stdafx.h"
#include "ParentReq.h"
#include "Core/StrBuf.h"
#include "Utils/Bitwise.h"
#include <climits>

namespace storm {
	namespace syntax {
		namespace glr {

			static const Nat bitsPerEntry = CHAR_BIT * sizeof(Nat);

			ParentReq::ParentReq() : data(null) {}

			ParentReq::ParentReq(Engine &e, Nat elem) {
				data = runtime::allocArray<Nat>(e, &natArrayType, elem/bitsPerEntry + 1);
				data->v[elem / bitsPerEntry] = Nat(1) << (elem % bitsPerEntry);
			}

			ParentReq::ParentReq(GcArray<Nat> *data) : data(data) {}

			Nat ParentReq::count() const {
				if (!data)
					return 0;

				Nat result = 0;
				for (Nat i = 0; i < data->count; i++)
					result += setBitCount(data->v[i]);

				return result;
			}

			Nat ParentReq::max() const {
				if (data)
					return Nat(data->count) * bitsPerEntry;
				else
					return 0;
			}

			Bool ParentReq::get(Nat id) const {
				if (id >= max())
					return false;
				return (data->v[id / bitsPerEntry] >> (id % bitsPerEntry)) & 1;
			}

			ParentReq ParentReq::concat(Engine &e, ParentReq other) const {
				if (empty())
					return other;
				if (other.empty())
					return *this;

				// Now, we know that both are non-empty!
				GcArray<Nat> *srcSmall = data;
				GcArray<Nat> *srcLarge = other.data;
				if (srcSmall->count > srcLarge->count)
					swap(srcSmall, srcLarge);

				GcArray<Nat> *r = runtime::allocArray<Nat>(e, &natArrayType, srcLarge->count);
				memcpy(r->v, srcLarge->v, srcLarge->count * sizeof(Nat));
				for (Nat i = 0; i < srcSmall->count; i++)
					r->v[i] |= srcSmall->v[i];

				return ParentReq(r);
			}

			ParentReq ParentReq::remove(Engine &e, ParentReq other) const {
				if (empty() || other.empty())
					return *this;

				// Check size of the resulting structure...
				Nat count = 0;
				Nat to = Nat(::min(data->count, other.data->count));
				for (Nat i = 0; i < to; i++) {
					if (data->v[i] & ~other.data->v[i])
						count = i + 1;
				}

				// Empty?
				if (count == 0)
					return ParentReq();

				GcArray<Nat> *r = runtime::allocArray<Nat>(e, &natArrayType, count);
				for (Nat i = 0; i < count; i++)
					r->v[i] = data->v[i] & ~other.data->v[i];

				return ParentReq(r);
			}

			Bool ParentReq::has(ParentReq other) const {
				if (other.empty())
					return true;
				if (empty())
					return false;
				if (other.data->count > data->count)
					return false;

				Nat to = Nat(other.data->count);
				for (Nat i = 0; i < to; i++) {
					if ((data->v[i] & other.data->v[i]) != other.data->v[i])
						return false;
				}

				return true;
			}

			Bool ParentReq::operator ==(const ParentReq &o) const {
				if (data && o.data) {
					if (data->count != o.data->count)
						return false;

					return memcmp(data->v, o.data->v, data->count * sizeof(Nat)) == 0;
				} else {
					// At least one of them is null.
					return data == o.data;
				}
			}

			wostream &operator <<(wostream &to, const ParentReq &r) {
				if (r.empty()) {
					to << L"{}";
				} else {
					to << L"{";
					for (Nat i = 0; i < r.max(); i++)
						if (r.get(i))
							to << L" " << i;
					to << L" }";
				}
				return to;
			}

			void ParentReq::toS(StrBuf *to) const {
				if (empty()) {
					*to << S("{}");
				} else {
					*to << S("{");
					for (Nat i = 0; i < max(); i++)
						if (get(i))
							*to << S(" ") << i;
					*to << S(" }");
				}
			}

		}
	}
}
