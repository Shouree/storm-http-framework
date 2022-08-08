#include "Utils/Platform.h"
#if defined(X64)

	.text

	.globl checkCall
	.type checkCall, @function
checkCall:
	# Prolog.
	push %rbp
	movq %rsp, %rbp

	# Allocate stack space
	subq $16, %rsp

	# Save the parameters we care about.
	movq %rdi, %rax # fn
	movq %rsi, %rdi # param
	movq %rdx, -16(%rbp) # regs

	# Read data into all registers that are supposed to be restored so they can be verified later.
	# Also, save the data from the preserved registers so we can restore them later.
	xchgq 0(%rdx), %rbx
	xchgq 8(%rdx), %r12
	xchgq 16(%rdx), %r13
	xchgq 24(%rdx), %r14
	xchgq 32(%rdx), %r15

	# Call the function.
	callq *%rax

	# Save data into 'reg', so we can verify the data. Also restore the previous values to the preserved registers.
	movq -16(%rbp), %rdx
	xchgq %rbx, 0(%rdx)
	xchgq %r12, 8(%rdx)
	xchgq %r13, 16(%rdx)
	xchgq %r14, 24(%rdx)
	xchgq %r15, 32(%rdx)

	movq %rbp, %rsp
	popq %rbp
	retq

#elif defined(ARM64)

	.text

	.globl checkCall
	.type checkCall, @function
checkCall:
	# TODO: Finish!
	ret

#endif

	# No executable stack.
	.section .note.GNU-stack,"",%progbits
