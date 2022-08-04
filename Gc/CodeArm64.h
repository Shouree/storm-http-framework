#pragma once

namespace storm {
	namespace arm64 {

		// Writing references for ARM.
		void writePtr(void *code, const GcCode *refs, Nat id);

		// Finalization for ARM64.
		inline bool needFinalization() { return true; }
		void finalize(void *code);

	}
}
