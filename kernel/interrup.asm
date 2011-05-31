;
; interrup.asm ~ Wrappers for ISR / IRQ handlers
;

%macro ISR_NOERRCODE 1
	push byte 0		; Dummy error code
	push byte %1
	jmp isr_common
%endmacro

%macro ISR_ERRCODE 1
	push byte %1
	jmp isr_common
%endmacro

%macro IRQ 2
global irq%1
irq%1:
	push byte %1
	push byte %2
	jmp irq_common
%endmacro


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

global sysint

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
sysint:  ISR_NOERRCODE 0x30

; Duplicated in kernel/idt.c
%define PIC1_START_IRQ 0x20
%define PIC2_START_IRQ 0x28

IRQ 0, (PIC1_START_IRQ + 0)
IRQ 1, (PIC1_START_IRQ + 1)
IRQ 2, (PIC1_START_IRQ + 2)
IRQ 3, (PIC1_START_IRQ + 3)
IRQ 4, (PIC1_START_IRQ + 4)
IRQ 5, (PIC1_START_IRQ + 5)
IRQ 6, (PIC1_START_IRQ + 6)
IRQ 7, (PIC1_START_IRQ + 7)

IRQ 8,  (PIC2_START_IRQ + 0)
IRQ 9,  (PIC2_START_IRQ + 1)
IRQ 10, (PIC2_START_IRQ + 2)
IRQ 11, (PIC2_START_IRQ + 3)
IRQ 12, (PIC2_START_IRQ + 4)
IRQ 13, (PIC2_START_IRQ + 5)
IRQ 14, (PIC2_START_IRQ + 6)
IRQ 15, (PIC2_START_IRQ + 7)


extern isr_handler, irq_handler

isr_common:
	pushad				; Push edi, esi, ebp, esp, ebx, edx, ecx, eax

	call isr_handler

	popad
	add esp, 8			; Cleans up the pushed error code and pushed ISR number
	iret


irq_common:
	pushad

	call irq_handler

	popad
	add esp, 8
	iret

; Spurious interrupts
global spurious_slave_handler
spurious_slave_handler:
	; Send EOI to Master PIC (port 0x20)
	mov dx, 0x20
	mov al, 0x20
	out dx, al
	iret

global spurious_master_handler
spurious_master_handler:
	iret
