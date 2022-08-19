#include "stdafx.h"
#include "Layout.h"
#include "../Listing.h"
#include "../Binary.h"
#include "../Layout.h"

namespace code {
	namespace arm64 {

		Layout::Layout(Binary *owner) : owner(owner) {}

		void Layout::before(Listing *dest, Listing *src) {
			PVAR(src);
			layout = code::arm64::layout(src, 0);

			PLN(L"Layout:");
			for (Nat i = 0; i < layout->count(); i++)
				PLN(i << L": " << layout->at(i));
		}

		void Layout::during(Listing *dest, Listing *src, Nat id) {}

		void Layout::after(Listing *dest, Listing *src) {
			*dest << alignAs(Size::sPtr);
			*dest << dest->meta();

			TODO(L"Add metadata!");

			// Owner.
			*dest << dat(objPtr(owner));
		}

		Array<Offset> *layout(Listing *src, /* Params *params, */ Nat spilled) {
			Array<Offset> *result = code::layout(src);

			return result;
		}
	}
}
