#include "Utils/Platform.h"
#ifdef X64	

	.text

	# Address of the logical return location inside 'doSwitch'. Used to get decend stack traces, not much more.
	# Used in the C++ code.
	.globl doSwitchReturnLoc

	.globl doSwitch
	.type doSwitch, @function
	.align 4
doSwitch:
	# NOTE: newDesc is in RDI
	# NOTE: oldDesc is in RSI

	# NOTE: We need CFI here to allow stack traces for all UThreads in the system.
	.cfi_startproc

	# Prolog. Bonus: saves RBP!
	push %rbp
	.cfi_def_cfa_offset 16
	.cfi_offset rbp, -16
	movq %rsp, %rbp

	# Save state. We're saving 'rdi' even though it is not strictly neccessary in order to call simple functions.
	pushq %rdi
	.cfi_def_cfa_offset 24
	.cfi_offset rdi, -24
	pushq %rbx
	.cfi_def_cfa_offset 32
	.cfi_offset rbx, -32
	pushq %r12
	.cfi_def_cfa_offset 40
	.cfi_offset r12, -40
	pushq %r13
	.cfi_def_cfa_offset 48
	.cfi_offset r13, -48
	pushq %r14
	.cfi_def_cfa_offset 56
	.cfi_offset r14, -56
	pushq %r15
	.cfi_def_cfa_offset 64
	.cfi_offset r15, -64

	# Dummy value to keep stack alignment when we're ready for a stack switch.
	pushq $0
	.cfi_def_cfa_offset 72

	# Save stack information. Creating a StackDesc structure on the stack.
	# We do not know anything about the stack on X86-64...
	pushq $0
	.cfi_def_cfa_offset 80
	pushq $0
	.cfi_def_cfa_offset 88
	# Current stack pointer, overlaps with 'low' in StackDesc.
	leaq -8(%rsp), %rbx
	pushq %rbx
	.cfi_def_cfa_offset 96

	# Report the state of the old thread. This makes it possible for the GC to scan that properly.
	movq %rsp, (%rsi)

	# Switch to the new stack.
	movq (%rdi), %rsp

	# This is the 'logical' location where threads are paused. We will use this as the "return address"
	# when using detours to get good stack traces.
doSwitchReturnLoc:

	# Skip the ThreadDesc struct.
	addq $32, %rsp

	.cfi_def_cfa_offset 64
	# Store newDesc in RAX so that we do not overwrite it.
	movq %rdi, %rax

	# Restore state.
	popq %r15
	.cfi_def_cfa_offset 56
	.cfi_restore r15
	popq %r14
	.cfi_def_cfa_offset 48
	.cfi_restore r14
	popq %r13
	.cfi_def_cfa_offset 40
	.cfi_restore r13
	popq %r12
	.cfi_def_cfa_offset 32
	.cfi_restore r12
	popq %rbx
	.cfi_def_cfa_offset 24
	.cfi_restore rbx
	popq %rdi
	.cfi_def_cfa_offset 16
	.cfi_restore rdi

	# Clear the new thread's description. This means that the GC will start scanning the
	# stack based on RSP rather than what is in the StackDesc.
	movq $0, (%rax)

	# Epilog. Bonus: restores RBP!
	pop %rbp
	.cfi_restore rbp
	.cfi_def_cfa rsp, 8
	retq
	.cfi_endproc
	
	.text
	# doEndDetour()
	.globl doEndDetour
	.type doEndDetour, @function
	.align 4 # To avoid being identified as a member function.
doEndDetour:
	# Reserve space for the additional return value.
	pushq $0
	mov %rsp, %rax

	# NOTE: Since the last 'parameter' to a function is known to be aligned at a 16 byte boundary, we know
	# that %rsp is now aligned on a 16 byte boundary as well. This alignment holds up all the way through this
	# function since we push exactly 6 words on the stack, followed by a couple of 16 byte XMM registers.

	# Save registers required for calling another function.
	pushq %rdi
	pushq %rsi
	pushq %rdx
	pushq %rcx
	pushq %r8
	pushq %r9

	# Floating-point registers as well.
	subq $16, %rsp
	movaps %xmm0, (%rsp)
	subq $16, %rsp
	movaps %xmm1, (%rsp)
	subq $16, %rsp
	movaps %xmm2, (%rsp)
	subq $16, %rsp
	movaps %xmm3, (%rsp)
	subq $16, %rsp
	movaps %xmm4, (%rsp)
	subq $16, %rsp
	movaps %xmm5, (%rsp)
	subq $16, %rsp
	movaps %xmm6, (%rsp)
	subq $16, %rsp
	movaps %xmm7, (%rsp)

	# Now, we have saved everything. Call the 'doEndDetour2' function in C to perform thread switches properly.
	movq %rax, %rdi
	call *doEndDetour2@GOTPCREL(%rip)

	# Restore the state we saved on the stack!
	movaps (%rsp), %xmm7
	addq $16, %rsp
	movaps (%rsp), %xmm6
	addq $16, %rsp
	movaps (%rsp), %xmm5
	addq $16, %rsp
	movaps (%rsp), %xmm4
	addq $16, %rsp
	movaps (%rsp), %xmm3
	addq $16, %rsp
	movaps (%rsp), %xmm2
	addq $16, %rsp
	movaps (%rsp), %xmm1
	addq $16, %rsp
	movaps (%rsp), %xmm0
	addq $16, %rsp

	popq %r9
	popq %r8
	popq %rcx
	popq %rdx
	popq %rsi
	popq %rdi

	# Let 'ret' handle the other one, which should contain an address now!
	retq

	# doEndDetourMember()
	# Like doEndDetour, but checks if a vtable lookup is required before issuing a 'ret' instruction.
	.globl doEndDetourMember
	.type doEndDetourMember, @function
	.align 4 # To avoid being identified as a member function.
doEndDetourMember:
	# Reserve space for the additional return value.
	pushq $0
	mov %rsp, %rax

	# NOTE: Since the last 'parameter' to a function is known to be aligned at a 16 byte boundary, we know
	# that %rsp is now aligned on a 16 byte boundary as well. This alignment holds up all the way through this
	# function since we push exactly 6 words on the stack, followed by a couple of 16 byte XMM registers.

	# Save registers required for calling another function.
	pushq %rdi
	pushq %rsi
	pushq %rdx
	pushq %rcx
	pushq %r8
	pushq %r9

	# Floating-point registers as well.
	subq $16, %rsp
	movaps %xmm0, (%rsp)
	subq $16, %rsp
	movaps %xmm1, (%rsp)
	subq $16, %rsp
	movaps %xmm2, (%rsp)
	subq $16, %rsp
	movaps %xmm3, (%rsp)
	subq $16, %rsp
	movaps %xmm4, (%rsp)
	subq $16, %rsp
	movaps %xmm5, (%rsp)
	subq $16, %rsp
	movaps %xmm6, (%rsp)
	subq $16, %rsp
	movaps %xmm7, (%rsp)

	# Now, we have saved everything. Call the 'doEndDetour2' function in C to perform thread switches properly.
	movq %rax, %rdi
	call *doEndDetour2@GOTPCREL(%rip)

	# Restore the state we saved on the stack!
	movaps (%rsp), %xmm7
	addq $16, %rsp
	movaps (%rsp), %xmm6
	addq $16, %rsp
	movaps (%rsp), %xmm5
	addq $16, %rsp
	movaps (%rsp), %xmm4
	addq $16, %rsp
	movaps (%rsp), %xmm3
	addq $16, %rsp
	movaps (%rsp), %xmm2
	addq $16, %rsp
	movaps (%rsp), %xmm1
	addq $16, %rsp
	movaps (%rsp), %xmm0
	addq $16, %rsp

	popq %r9
	popq %r8
	popq %rcx
	popq %rdx
	popq %rsi
	popq %rdi

	# See if we are about to return to a member function. We can only trash %rax now!
	movq (%rsp), %rax

	# If it is odd, we need to do a vtable lookup!
	testq $1, %rax
	je 1f

	# Perform a vtable lookup and store it on the stack, ready for 'retq' to consume.
	movq (%rdi), %rax  # load the vtable pointer
	addq (%rsp), %rax  # add the offset
	subq $1, %rax      # remove the marker
	movq (%rax), %rax  # read the function pointer
	movq %rax, (%rsp)  # and save it to the stack for 'retq'

1:
	# Let 'ret' handle the other one, which should contain an address now!
	retq

#endif

	# No executable stack.
	.section .note.GNU-stack,"",%progbits
