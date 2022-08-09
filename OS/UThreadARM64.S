#include "Utils/Platform.h"
#ifdef ARM64

	.text

	# TODO: We need stack-switching logic for ARM here.

	# ABI: Registers x19-x28 needs to be preserved.
	# Registers x29 and x30 are the frame- and link pointers. Also need preservation,
	# but this is done in "standard" prolog/epilog.

#endif

	# No executable stack.
	.section .note.GNU-stack,"",%progbits
