#include "Utils/Platform.h"
#ifdef ARM64

	.text

	.globl doStackCall
	.type doStackCall, @function
	.align 4
doStackCall:
	.cfi_startproc
	# TODO: We need stack-switching logic for ARM here.
	ret
	.cfi_endproc

#endif

	# No executable stack.
	.section .note.GNU-stack,"",%progbits
