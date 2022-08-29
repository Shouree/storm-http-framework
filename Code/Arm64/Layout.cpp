#include "stdafx.h"
#include "Layout.h"
#include "../Listing.h"
#include "../Binary.h"
#include "../Layout.h"

namespace code {
	namespace arm64 {

		Layout::Layout(Binary *owner) : owner(owner) {}

		void Layout::before(Listing *dest, Listing *src) {
			// Find parameters.
			Array<Var> *p = src->allParams();
			params = new (this) Params();
			for (Nat i = 0; i < p->count(); i++) {
				params->add(i, src->paramDesc(p->at(i)));
			}

			PVAR(src);
			layout = code::arm64::layout(src, params, 0);

			PLN(L"Layout:");
			for (Nat i = 0; i < layout->count(); i++)
				PLN(i << L": " << layout->at(i));
		}

		void Layout::during(Listing *dest, Listing *src, Nat id) {
			*dest << src->at(id);
		}

		void Layout::after(Listing *dest, Listing *src) {
			*dest << alignAs(Size::sPtr);
			*dest << dest->meta();

			TODO(L"Add metadata!");

			// Owner.
			*dest << dat(objPtr(owner));
		}

		Array<Offset> *layout(Listing *src, Params *params, Nat spilled) {
			Array<Offset> *result = code::layout(src);

			// TODO: Look at parameters and figure out where they are, and how they should be spilled to the stack.

			// TODO: Layout data on the stack.

			return result;
		}
	}
}
