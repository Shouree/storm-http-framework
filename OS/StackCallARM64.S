#include "Utils/Platform.h"
#ifdef ARM64

	.text

	# TODO: We need stack-switching logic for ARM here.

#endif

	# No executable stack.
	.section .note.GNU-stack,"",%progbits
