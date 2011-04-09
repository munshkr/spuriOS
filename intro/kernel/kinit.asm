BITS 16
extern kernel_start
extern GDT_DESC
global start
start: 
	call enable_A20

	lgdt [GDT_DESC]

	mov eax, cr0
	or eax, 0x1
	mov cr0, eax

	jmp 0x08:modo_protegido

BITS 32
modo_protegido:
	mov 	ax, 0x10
	mov 	ds, ax
	mov 	es, ax
	mov 	fs, ax
	mov 	ss, ax
	mov 	esp, 0xA0000
	mov 	ebp, 0xA0000

	call 	kernel_start
	jmp $

%include "a20.asm"
