;; isr.asm
;;

extern debug_kernelpanic
global isr_panic

isr_panic:
	pusha

	push dword 0
	push dword 0
	call debug_kernelpanic
	add esp, 8

	popad
	iret
