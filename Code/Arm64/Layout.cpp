#include "stdafx.h"
#include "Layout.h"
#include "../Listing.h"
#include "../Binary.h"

namespace code {
	namespace arm64 {

		Layout::Layout(Binary *owner) : owner(owner) {}

		void Layout::before(Listing *dest, Listing *src) {}

		void Layout::during(Listing *dest, Listing *src, Nat id) {}

		void Layout::after(Listing *dest, Listing *src) {
			*dest << alignAs(Size::sPtr);
			*dest << dest->meta();

			TODO(L"Add metadata!");

			// Owner.
			*dest << dat(objPtr(owner));
		}

	}
}
