.code

deckard_has_cpuid PROC PUBLIC
	push rbx
	pushfq
	pop rax
	mov rbx, rax
	xor rax, 200000h
	push rax
	popfq
	pushfq
	pop rax
	cmp rax, rbx
	jz not_supported
	mov eax, 1
	jmp exit
not_supported:
	xor eax, eax
exit:
	pop rbx
	ret
deckard_has_cpuid ENDP

END
