;; isr.asm
;;

extern debug_kernelpanic
global isr_de
global isr_db
global isr_nmi
global isr_bp
global isr_of
global isr_br
global isr_ud
global isr_nm
global isr_df
global isr_cso
global isr_ts
global isr_np
global isr_ss
global isr_gp
global isr_pf
global isr_mf
global isr_ac
global isr_mc
global isr_xm

%define WITH_ERR_CODE(x) (x + 0x80000000)

; Takes de interrupt number
%macro PANIC_ISR 1
	pushad

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
isr_db:
	PANIC_ISR 1
isr_nmi:
	PANIC_ISR 2
isr_bp:
	PANIC_ISR 3
isr_of:
	PANIC_ISR 4
isr_br:
	PANIC_ISR 5
isr_ud:
	PANIC_ISR 6
isr_nm:
	PANIC_ISR 7
isr_df:
	PANIC_ISR WITH_ERR_CODE(8)
isr_cso:
	PANIC_ISR 9
isr_ts:
	PANIC_ISR WITH_ERR_CODE(10)
isr_np:
	PANIC_ISR WITH_ERR_CODE(11)
isr_ss:
	PANIC_ISR WITH_ERR_CODE(12)
isr_gp:
	PANIC_ISR WITH_ERR_CODE(13)
isr_pf:
	PANIC_ISR WITH_ERR_CODE(14)
isr_mf:
	PANIC_ISR 16
isr_ac:
	PANIC_ISR WITH_ERR_CODE(17)
isr_mc:
	PANIC_ISR 18
isr_xm:
	PANIC_ISR 19
