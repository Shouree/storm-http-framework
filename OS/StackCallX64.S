#include "Utils/Platform.h"
#ifdef X64

	.text

	.globl doStackCall
	.type doStackCall, @function
	.align 4
doStackCall:
	# 'current' is in %rdi
	# 'callOn' is in %rsi
	# 'fn' is in %rdx
	# 'fnCall' is in %rcx
	# 'member' is in %r8
	# 'result' is in %r9

	# We may use %rax, %r10 and %r11 as a scratchpad
	.cfi_startproc

	# Prolog. Saves %rsp.
	push %rbp
	.cfi_def_cfa rsp, 16
	.cfi_offset rbp, -16
	movq %rsp, %rbp
	.cfi_def_cfa rbp, 16

	# We need to save 'current' for after the thread call.
	push %rdi

	# First, we need to make the old stack scannable. Thus, we need to store a Stack::Desc struct there.
	pushq $0 		# high
	pushq $0		# dummy
	leaq -8(%rsp), %rax
	pushq %rax		# low
	
	movq %rsp, 16(%rdi)	# current.desc = %rsp
	movq %rsi, 32(%rdi)	# current.detourTo = callOn

	# Now, we can switch to the other stack.
	movq 16(%rsi), %rax 	# %rax = callOn.desc
	movq (%rax), %rsp	# %rsp = desc.low
	movq $0, 16(%rsi)	# callOn.desc = null

	# Finally, call 'os::FnCallRaw::callRaw'
	movq %rcx, %rdi 	# this ptr
	movq %rdx, %rsi 	# fn
	movq %r8, %rdx		# member
	movq $0, %rcx		# first
	movq %r9, %r8		# result
	callq *fnCallRaw@GOTPCREL(%rip)

	# Done! Tear down. Start by unlinking the old stack, otherwise the GC might scan too much.
	# Note that we can not set "desc" again, as we have lost the pointer to it.
	movq -8(%rbp), %rdi	# Load "current" back into %rdi.
	movq $0, 32(%rdi)	# current.detourTo = null
	movq %rbp, %rsp		# Reset %rsp, it is safe now.
	movq $0, 16(%rdi)	# current.desc = null

	# Epilog!
	pop %rbp
	.cfi_restore rbp
	.cfi_def_cfa rsp, 8
	retq
	.cfi_endproc

#endif

	# No executable stack.
	.section .note.GNU-stack,"",%progbits
