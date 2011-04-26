;
; interrup.asm ~ Wrappers for ISR / IRQ handlers
;

section .text

global idt_de
global idt_db
global idt_nmi
global idt_bp
global idt_of
global idt_br
global idt_ud
global idt_nm
global idt_df
global idt_cso
global idt_ts
global idt_np
global idt_ss
global idt_gp
global idt_pf
global idt_mf
global idt_ac
global idt_mc
global idt_xm


%define WITH_ERR_CODE(x) (x + 0x80000000)

%macro ISR_NOERRCODE 1
	cli
	push byte 0		; Dummy error code
	push byte %1
	jmp isr_common
%endmacro

%macro ISR_ERRCODE 1
	cli
	push byte %1
	jmp isr_common
%endmacro

%macro IRQ 2
global irq%1
irq%1:
	cli
	push byte %1
	push byte %2
	jmp irq_common
%endmacro


idt_de:  ISR_NOERRCODE  0
idt_db:  ISR_NOERRCODE  1
idt_nmi: ISR_NOERRCODE  2
idt_bp:  ISR_NOERRCODE  3
idt_of:  ISR_NOERRCODE  4
idt_br:  ISR_NOERRCODE  5
idt_ud:  ISR_NOERRCODE  6
idt_nm:  ISR_NOERRCODE  7
idt_df:  ISR_ERRCODE    8
idt_cso: ISR_NOERRCODE  9
idt_ts:  ISR_ERRCODE   10
idt_np:  ISR_ERRCODE   11
idt_ss:  ISR_ERRCODE   12
idt_gp:  ISR_ERRCODE   13
idt_pf:  ISR_ERRCODE   14
idt_mf:  ISR_NOERRCODE 16
idt_ac:  ISR_NOERRCODE 17
idt_mc:  ISR_ERRCODE   18
idt_xm:  ISR_NOERRCODE 19

; TODO define IRQ handlers
; ...

extern isr_handler, irq_handler

isr_common:
	pushad				; Push edi, esi, ebp, esp, ebx, edx, ecx, eax

	mov ax, ds
	push eax

	mov ax, 0x10		; Load kernel data segment descriptor in all selectors
	mov ds, ax
	mov es, ax
	mov fs, ax
	mov gs, ax

	call isr_handler
	;
	; TODO look for handler in array and call function
	; ...
	;mov esi, [
	;mov eax, [handlers + esi

	pop eax				; Reload the original data segment descriptor
	mov ds, ax
	mov es, ax
	mov fs, ax
	mov gs, ax

	popad
	add esp, 8			; Cleans up the pushed error code and pushed ISR number
	sti
	iret


irq_common:
	pushad

	mov ax, ds
	push eax

	mov ax, 0x10
	mov ds, ax
	mov es, ax
	mov fs, ax
	mov gs, ax

	call irq_handler
	;
	; TODO send EOI, and call function from handler's array
	; ...

	pop eax
	mov ds, ax
	mov es, ax
	mov fs, ax
	mov gs, ax

	popad
	add esp, 8
	sti
	iret


section .bss
; TODO
;handlers: resd 256
