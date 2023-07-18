#pragma once
#include "GcArray.h"
#include <climits>

namespace storm {

	struct GcBitset : private GcArray<byte> {
		inline nat count() const {
			return nat(filled);
		}

		inline bool has(nat bit) const {
			const byte &d = v[bit / CHAR_BIT];
			return ((d >> (bit % CHAR_BIT)) & 0x1) != 0;
		}

		inline void set(nat bit, bool to) {
			byte &d = v[bit / CHAR_BIT];
			byte mask = 1 << (bit % CHAR_BIT);
			if (to)
				d |= mask;
			else
				d &= ~mask;
		}

		inline void clear() {
			for (nat i = 0; i < GcArray<byte>::count; i++)
				v[i] = 0;
		}

		inline void fill() {
			for (nat i = 0; i < GcArray<byte>::count; i++)
				v[i] = ~byte(0);
		}

		friend GcBitset *allocBitset(Engine &e, nat count);
	};

	// GcType for the bit set.
	extern const GcType bitsetType;

	// Allocate a bitset.
	GcBitset *allocBitset(Engine &e, nat count);

}
