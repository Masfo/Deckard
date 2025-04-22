section .text
bits 64
global has_cpuid

bits 32
has_cpuid:
	push ebx
	pushfd
	pop eax
	mov ebx, eax
	xor eax, 200000h ; bit 21
	push eax
	popfd
	pushfd
	pop eax
	cmp eax, ebx
	jz not_supported
	mov eax, 1
	jmp exit
not_supported:
	xor eax,eax
exit:
	pop ebx
	ret