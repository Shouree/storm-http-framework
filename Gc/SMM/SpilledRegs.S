#include "Utils/Platform.h"
#ifdef X64

	.text

	.globl spillRegs
	.type spillRegs, @function
spillRegs:
	# This is for Linux (SystemV ABI, for Windows, we need something else)
	# rdi is the first parameter!
	movq %rbx, 0x00(%rdi)
	movq %rbp, 0x08(%rdi)
	movq %r10, 0x10(%rdi)
	movq %r11, 0x18(%rdi)
	movq %r12, 0x20(%rdi)
	movq %r13, 0x28(%rdi)
	movq %r14, 0x30(%rdi)
	movq %r15, 0x38(%rdi)
	retq

#endif

	# No executable stack.
	.section .note.GNU-stack,"",%progbits
