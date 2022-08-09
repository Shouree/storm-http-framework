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

	# We can use x9-x15

	# We need to preserve registers x19-x28
	# x29 and x30 is the frame- and link register. x30 is the register used upon return.

	.cfi_startproc
	# Store stack and frame pointers to the stack. Also decrements the stack.
	# We allocate enough memory for everything here.
	stp x29, x30, [sp, -48]!
	.cfi_def_cfa_offset 48
	.cfi_offset 29, -48
	.cfi_offset 30, -40
	# Store frame pointer (we might be able to ignore this)
	mov x29, sp
	.cfi_def_cfa x29, 48

	# We create the following structure on the stack:
	# +40:  current (for recovery)
	# +32:  desc.high
	# +24:  desc.dummy
	# +16:  desc.low
	#  +8:  x30
	#  +0:  x29

	stp xzr, xzr, [sp, 16]
	add x9, sp, #16
	stp x9, x0, [sp, 32]

	# Store it in the current thread.
	str x9, [x0, 16] // current.desc = "addr of Stack::Desc" struct.
	str x1, [x0, 32] // current.detourTo = callOn

	# Now we can switch to the other stack.
	ldr x9, [x1, 16] // x9 = callOn.desc
	ldr x9, [x9]     // x9 = callOn.desc.low
	mov sp, x9       // sp = x9
	str xzr, [x1, 16] // callOn.desc = null

	# Call 'os::FnCallRaw::callRaw'
	mov x0, x3
	mov x1, x2
	mov x2, x4
	mov x3, xzr
	mov x4, x5
	bl fnCallRaw

	# Done! Tear down. Start by unlinking the old stack. Otherwise the GC might scan too much.
	ldr x0, [x29, 40]  // Load 'current' from stack. Note: sp is trashed, so we use the frame pointer, x29
	str xzr, [x0, 32] // current.detourTo = null
	mov sp, x29       // restore sp
	str xzr, [x0, 16] // current.desc = null

	# Restore stack and frame offset, and restore sp.
	ldp x29, x30, [sp], 48
	.cfi_def_cfa_register sp
	.cfi_restore 29
	.cfi_restore 30
	.cfi_def_cfa_offset 0
	ret
	.cfi_endproc

#endif

	# No executable stack.
	.section .note.GNU-stack,"",%progbits
