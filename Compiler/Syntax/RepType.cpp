#include "stdafx.h"
#include "RepType.h"

namespace storm {
	namespace syntax {

		Bool skippable(RepType r) {
			switch (r) {
			case repZeroOne:
			case repZeroPlus:
				return true;
			default:
				return false;
			}
		}

		Bool repeatable(RepType r) {
			switch (r) {
			case repOnePlus:
			case repZeroPlus:
				return true;
			default:
				return false;
			}
		}
	}
}
