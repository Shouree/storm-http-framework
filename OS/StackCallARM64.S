#include "Utils/Platform.h"
#ifdef ARM64

	.text

	.globl doStackCall
	.type doStackCall, @function
	.align 4
doStackCall:
	# 'current' is in x0
	# 'callOn' is in x1
	# 'fn' is in x2
	# 'fnCall' is in x3
	# 'member' is in x4
	# 'result' is in x5

	.cfi_startproc

	ret
	.cfi_endproc

#endif

	# No executable stack.
	.section .note.GNU-stack,"",%progbits
