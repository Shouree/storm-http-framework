#include "Utils/Platform.h"
#ifdef X64

	.text

	.globl x64ResumeExceptionThunk
	.type x64ResumeExceptionThunk, @function
	.align 4

x64ResumeExceptionThunk:
	# Save registers we need. Note: We don't really need to save anything except for what
	# we need later on.
	pushq %rdx
	pushq $0

	# Call the resumeexception function. Second parameter is pointer to topmost element
	# on the stack so that we can load our stack pointer later on.
	movq %rax, %rdi
	movq %rsp, %rsi

	call *x86ResumeException@GOTPCREL(%rip)

	# Restore and jump.
	popq %rcx
	popq %rdx

	# Update stack pointer and continue!
	mov %rcx, %rsp
	jmp *%rdx

#endif

	# No executable stack.
	.section .note.GNU-stack,"",%progbits
