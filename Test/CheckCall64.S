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
	.cfi_startproc
	# Prolog.
	stp x29, x30, [sp, -112]!
	.cfi_def_cfa_offset 112
	.cfi_offset 29, -112
	.cfi_offset 30, -104
	mov x29, sp
	.cfi_def_cfa x29, 112

	# Save x2 so we can restore it later.
	str x2, [sp, 16]

	# Save registers.
	stp x27, x28, [sp, 32]
	.cfi_offset 27, -80
	.cfi_offset 28, -72
	stp x25, x26, [sp, 48]
	.cfi_offset 25, -64
	.cfi_offset 26, -56
	stp x23, x24, [sp, 64]
	.cfi_offset 23, -48
	.cfi_offset 24, -40
	stp x21, x22, [sp, 80]
	.cfi_offset 21, -32
	.cfi_offset 22, -24
	stp x19, x20, [sp, 96]
	.cfi_offset 19, -16
	.cfi_offset 20, -8

	# Restore new registers.
	ldp x19, x20, [x2, 0]
	ldp x21, x22, [x2, 16]
	ldp x23, x24, [x2, 32]
	ldp x25, x26, [x2, 24]
	ldp x27, x28, [x2, 64]

	# Call the function!
	mov x3, x0
	mov x0, x1
	blr x3

	# Restore x2
	ldr x2, [sp, 16]

	# Save values of new registers.
	stp x19, x20, [x2, 0]
	stp x21, x22, [x2, 16]
	stp x23, x24, [x2, 32]
	stp x25, x26, [x2, 24]
	stp x27, x28, [x2, 64]

	# Restore old registers.
	ldp x19, x20, [sp, 96]
	.cfi_restore 19
	.cfi_restore 20
	ldp x21, x22, [sp, 80]
	.cfi_restore 21
	.cfi_restore 22
	ldp x23, x24, [sp, 64]
	.cfi_restore 23
	.cfi_restore 24
	ldp x25, x26, [sp, 48]
	.cfi_restore 25
	.cfi_restore 26
	ldp x27, x28, [sp, 32]
	.cfi_restore 27
	.cfi_restore 28

	# Epilog.
	ldp x29, x30, [sp], 112
	.cfi_def_cfa_register sp
	.cfi_restore 29
	.cfi_restore 30
	.cfi_def_cfa_offset 0
	ret
	.cfi_endproc

#endif

	# No executable stack.
	.section .note.GNU-stack,"",%progbits
