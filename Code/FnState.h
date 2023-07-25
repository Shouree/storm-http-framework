#pragma once

namespace code {

	// Encode/decode function state. The function state is a block-id and an activation-id
	// inside a single 32-bit integer. Designed so that the value can be updated easily from
	// machine code. This is why it is a 32-bit integer rather than a 64-bit integer.
	Nat encodeFnState(Nat block, Nat activation);
	void decodeFnState(Nat value, Nat &block, Nat &activation);

}

