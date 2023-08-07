#include "Utils/Platform.h"
#ifdef ARM64

	.text

	# Address of the logical return location inside 'doSwitch'. Used to get decent stack traces, not much more.
	# Used in the C++ code.
	.globl doSwitchReturnLoc

	.globl doSwitch
	.type doSwitch, @function
	.align 4
doSwitch:
	# Note: newDesc is in x0
	# Note: oldDesc is in x1

	# ABI: Regisers x19-x30 need to be preserved. Also the fp regs v8-v15.
	# ABI: Registers x29 and x30 are frame- and link pointers.

	# Total number of registers: 12 + 8. Total: 160 bytes.
	# We also need to allocate a StackDesc on the stack, which uses 3 words.

	# Stack layout:
	#  -0 24 +192: (end)
	#  -1 23 +184: (unused)
	#  -2 22 +176: Stack::Desc::hi
	#  -3 21 +168: Stack::Desc::dummy
	#  -4 20 +160: Stack::Desc::low
	#  -5 19 +152: v8
	#  -6 18 +144: v9
	#  -7 17 +136: v10
	#  -8 16 +128: v11
	#  -9 15 +120: v12
	# -10 14 +112: v13
	# -11 13 +104: v14
	# -12 12  +96: v15
	# -13 11  +88: x19
	# -14 10  +80: x20
	# -15  9  +72: x21
	# -16  8  +64: x22
	# -17  7  +56: x23
	# -18  6  +48: x24
	# -19  5  +40: x25
	# -20  4  +32: x26
	# -21  3  +24: x27
	# -22  2  +16: x28
	# -23  1   +8: x30  (to use "standard" layout)
	# -24  0   +0: x29  (to use "standard" layout), sp points here

	.cfi_startproc

	# Store link and frame pointer.
	stp x29, x30, [sp, -24*8]!
	.cfi_def_cfa_offset 24*8
	.cfi_offset 29, -24*8
	.cfi_offset 30, -23*8

	# Store base pointer.
	mov x29, sp

	# Save state.
	stp x28, x27, [sp, 2*8]
	.cfi_offset 27, -21*8
	.cfi_offset 28, -22*8
	stp x26, x25, [sp, 4*8]
	.cfi_offset 26, -20*8
	.cfi_offset 25, -19*8
	stp x24, x23, [sp, 6*8]
	.cfi_offset 24, -18*8
	.cfi_offset 23, -17*8
	stp x22, x21, [sp, 8*8]
	.cfi_offset 22, -16*8
	.cfi_offset 21, -15*8
	stp x20, x19, [sp, 10*8]
	.cfi_offset 20, -14*8
	.cfi_offset 19, -13*8

	stp d15, d14, [sp, 12*8]
	.cfi_offset d15, -12*8
	.cfi_offset d14, -11*8
	stp d13, d12, [sp, 12*8]
	.cfi_offset d13, -10*8
	.cfi_offset d12, -9*8
	stp d11, d10, [sp, 12*8]
	.cfi_offset d11, -8*8
	.cfi_offset d10, -7*8
	stp d9, d8, [sp, 12*8]
	.cfi_offset d9, -6*8
	.cfi_offset d8, -5*8

	# Create a StackDesc structure on the stack.
	mov x9, sp
	stp x9, xzr, [sp, 20*8]
	stp xzr, xzr, [sp, 22*8]

	# Report state to the old thread. This makes it possible for the GC to scan that properly.
	add x2, sp, 20*8  // compute pointer to structure
	str x2, [x1]

	# Switch to the new stack.
	ldr x2, [x0]  // read new desc.
	ldr x2, [x2]  // read new desc's 'low', which is the sp.
	mov sp, x2

	# This is the "logical" location where threads are paused. Used as the "return address" when using detours to get good stack traces.
doSwitchReturnLoc:

	# Start reading back old state.
	ldp x28, x27, [sp, 2*8]
	.cfi_restore 27
	.cfi_restore 28
	ldp x26, x25, [sp, 4*8]
	.cfi_restore 26
	.cfi_restore 25
	ldp x24, x23, [sp, 6*8]
	.cfi_restore 24
	.cfi_restore 23
	ldp x22, x21, [sp, 8*8]
	.cfi_restore 22
	.cfi_restore 21
	ldp x20, x19, [sp, 10*8]
	.cfi_restore 20
	.cfi_restore 19

	ldp d15, d14, [sp, 12*8]
	.cfi_restore d15
	.cfi_restore d14
	ldp d13, d12, [sp, 12*8]
	.cfi_restore d13
	.cfi_restore d12
	ldp d11, d10, [sp, 12*8]
	.cfi_restore d11
	.cfi_restore d10
	ldp d9, d8, [sp, 12*8]
	.cfi_restore d9
	.cfi_restore d8

	# Clear the new thread's description. The GC will start scanning the stack based on RSP rather than what is in the StackDesc.
	str xzr, [x0]

	# Restore link and base pointer.
	ldp x29, x30, [sp], 24*8
	.cfi_restore 29
	.cfi_restore 30

	# Done!
	ret
	.cfi_endproc


	# Helper ASM stub to launch a new thread.
	# This is not a function that follows the calling convention since we can't set
	# the appropriate registers from "doSwitch" (that ability would cost os cycles
	# in the general case, so it is not desirable to fix).
	#
	# This stub calls the function in x28 with the parameter in x27.
	# It makes it appear as if the function was called from address in x26.
	.globl launchThreadStub
	.type launchThreadStub, @function
	.align 4
launchThreadStub:
	mov x0, x27
	mov x27, xzr // clear saved regs to avoid accidental GC retention
	mov x30, x26
	mov x26, xzr // clear saved regs to avoid accidental GC retention
	br x28


	# Note: Uses x10 to determine whether to use member call or non-member-call.
	.globl doEndDetourCommon
	.type doEndDetourCommon, @function
	.align 4
doEndDetourCommon:

	# Stack layout:
	# 22: (size)
	# 21: (unused)
	# 20: (jump to)
	# 19: v7
	# 18: v6
	# 17: v5
	# 16: v4
	# 15: v3
	# 14: v2
	# 13: v1
	# 12: v0
	# 11: x10 (use member calls?)
	# 10: x8  (might not be strictly necessary, but it is the address of the return value)
	#  9: x7
	#  8: x6
	#  7: x5
	#  6: x4
	#  5: x3
	#  4: x2
	#  3: x1
	#  2: x0
	#  1: x30
	#  0: x29

	.cfi_startproc
	stp x29, x30, [sp, -22*8]!
	.cfi_def_cfa_offset 22*8
	.cfi_offset 29, -22*8
	.cfi_offset 30, -21*8

	# Save everything on the stack.
	stp x0, x1, [sp, 2*8]
	stp x2, x3, [sp, 4*8]
	stp x4, x5, [sp, 6*8]
	stp x6, x7, [sp, 8*8]
	stp x8, x10, [sp, 10*8]
	stp d0, d1, [sp, 12*8]
	stp d2, d3, [sp, 14*8]
	stp d4, d5, [sp, 16*8]
	stp d6, d7, [sp, 18*8]

	# Everything is saved. Call doEndDetour2 in C to do thread switching!
	add x0, sp, 20*8
	bl doEndDetour2

	# Restore.
	ldp x0, x1, [sp, 2*8]
	ldp x2, x3, [sp, 4*8]
	ldp x4, x5, [sp, 6*8]
	ldp x6, x7, [sp, 8*8]
	ldp x8, x10, [sp, 10*8]
	ldp d0, d1, [sp, 12*8]
	ldp d2, d3, [sp, 14*8]
	ldp d4, d5, [sp, 16*8]
	ldp d6, d7, [sp, 18*8]

	# Load where to jump to.
	ldr x11, [sp, 20*8]

	# Epilog.
	ldp x29, x30, [sp], 22*8

	# Now, we can call the function!
	
	# First, check if this is a non-member function.
	cmp x10, #0
	b.eq 1f

	# Might be a member function. Check LSB of pointer. If it is zero, it is a regular pointer that we can just call.
	tst x11, #1
	b.eq 1f

	# We need to do a vtable lookup to find the vtable.
	ldr x10, [x0]
	add x10, x10, x11
	sub x10, x10, #1  // remove tag
	ldr x11, [x10]

	# This is a regular function. Just call it!
1:	br x11
	.cfi_endproc

	.globl doEndDetour
	.type doEndDetour, @function
	.align 4
doEndDetour:
	mov x10, #0
	b doEndDetourCommon

	.globl doEndDetourMember
	.type doEndDetourMember, @function
	.align 4
doEndDetourMember:
	mov x10, #1
	b doEndDetourCommon
	
#endif

	# No executable stack.
	.section .note.GNU-stack,"",%progbits
