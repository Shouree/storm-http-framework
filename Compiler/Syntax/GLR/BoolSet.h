#pragma once
#include "Core/Object.h"
#include "Core/GcArray.h"
#include <climits>

namespace storm {
	namespace syntax {
		namespace glr {
			STORM_PKG(lang.bnf.glr);

			/**
			 * Space efficient boolean set.
			 */
			class BoolSet : public Object {
				STORM_CLASS;
			public:
				// Create.
				STORM_CTOR BoolSet();

				// Get a value.
				Bool STORM_FN get(Nat id);

				// Set a value.
				void STORM_FN set(Nat id, Bool v);

				// Clear all positions.
				void STORM_FN clear();

			private:
				// Data.
				GcArray<Nat> *data;

				// Constants.
				enum {
					bitsPerEntry = sizeof(Int) * CHAR_BIT,
					minSize = 8
				};

				// Current count.
				inline Nat count() const {
					return data ? Nat(data->count*bitsPerEntry) : 0;
				}

				// Compute bit entry and offset.
				inline Nat entry(Nat pos) {
					// Note: this could be a right-shift.
					return pos / bitsPerEntry;
				}

				inline Nat offset(Nat pos) {
					return pos & (bitsPerEntry - 1);
				}

				// Grow.
				void grow(Nat count);
			};

		}
	}
}
