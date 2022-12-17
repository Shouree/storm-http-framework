#include "Utils/Platform.h"
	.text

	.globl posixResumeExceptionThunk
	.type posixResumeExceptionThunk, @function
	.align 4

posixResumeExceptionThunk:

#if defined(X64)
	# Save registers we need. Note: We don't really need to save anything except for what
	# we need later on.
	pushq %rdx
	pushq $0

	# Call the resumeexception function. Second parameter is pointer to topmost element
	# on the stack so that we can load our stack pointer later on.
	movq %rax, %rdi
	movq %rsp, %rsi

	call *posixResumeException@GOTPCREL(%rip)

	# Restore and jump.
	popq %rcx
	popq %rdx

	# Update stack pointer and continue!
	mov %rcx, %rsp
	jmp *%rdx

#elif defined(ARM64)

	# Store what we need for later. The important part is that we need to be able to access
	# x1 later, to branch there.
	stp xzr, x1, [sp, -16]!

	# Call the resume function. As a parameter it needs the current exception, and a pointer
	# to where it should store the stack pointer. x0 is already in the right place.
	mov x1, sp
	bl posixResumeException

	# Restore state, update sp and jump. We may not trash x0 here.
	ldp x2, x1, [sp]
	mov sp, x2
	br x1

#endif

	# No executable stack.
	.section .note.GNU-stack,"",%progbits
