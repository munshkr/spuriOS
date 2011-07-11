global ap_trampoline

ALIGN 4096
ap_trampoline:
	xchg bx, bx
	jmp $
