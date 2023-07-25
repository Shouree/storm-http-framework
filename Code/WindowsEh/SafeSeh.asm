.386
.model flat, stdcall

extern x86SEH:near
extern x86SEHCleanup:near
extern RtlUnwind@16:near

x86SafeSEH proto
.SAFESEH x86SafeSEH

x86EhEntry proto	
x86Unwind proto C

.code
x86SafeSEH proc
	jmp x86SEH
x86SafeSEH endp

x86EhEntry proc
	;; Restore the EH chain.
	ASSUME FS:NOTHING
	mov DWORD PTR fs:[0], ecx
	ASSUME FS:ERROR
	;; Return to
	push edx
	;; Exception, param 3
	push eax
	;; Current part, param 2
	push ebx
	;; EH frame, param 1
	push ecx
	call x86SEHCleanup
	;; Remove parameters
	add esp, 12
	;; Consume the last push.
	ret
x86EhEntry endp

x86Unwind proc C
	push ebp
	mov ebp, esp

	;; RtlUnwind does not save registers... We need to do that!
	;; These are the callee-saved registers we need to care about.
	push ebx
	push edi
	push esi

	push 0
	push DWORD PTR [ebp+8]
	push x86UnwindResume
	push DWORD PTR [ebp+12]
	call RtlUnwind@16
x86UnwindResume:

	pop esi
	pop edi
	pop ebx
	
	mov ebp, esp
	pop ebp
	ret
x86Unwind endp

end
