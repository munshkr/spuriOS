global test_and_set
global signal

%define SPINLOCK [ebp + 8]
test_and_set:
	push	ebp
	mov		ebp, esp
	push	ebx
	push	esi
	push	edi
	
	mov		esi, SPINLOCK
	mov		al, 1
	xchg	al, [esi]

	pop		edi
	pop		esi
	pop		ebx
	pop		ebp
	ret

signal:
	push	ebp
	mov		ebp, esp
	push	ebx
	push	esi
	push	edi

	mov		esi, SPINLOCK
	mov		al, 0
	xchg	al, [esi]

	pop		edi
	pop		esi
	pop		ebx
	pop		ebp
	ret
