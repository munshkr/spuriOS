;; isr.asm
;;

extern debug_kernelpanic
global isr_de

%define WITH_ERR_CODE(x) (x + 0x80000000)

; Takes de interrupt number
%macro PANIC_ISR 1
	pusha

	; Pointer to the beginning of the pushed state
	push dword esp

	; Interrupt number
	push dword %1

	call debug_kernelpanic
	add esp, 8

	popad
	iret
%endmacro

isr_de:
	PANIC_ISR 0
