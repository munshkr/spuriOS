%include "bios.mac"

global start
global mmap_buffer

extern kernel_init
extern enable_A20
extern gdt

; both defined in mmap.asm
extern make_mmap
extern mmap_entries

section .text

bits 16


; start MUST be at the very begining of this file
start:
	call enable_A20
	xor ax, ax			; Clean ES selector as it is used
	mov es, ax			; by E820 for memory map.

memory_map:
	call make_mmap
	or eax, eax
	jnz .mmap_ok

.halt:
	hlt
	jmp .halt

.mmap_ok:
	lgdt [gdt_desc]

	mov eax, cr0
	or eax, 0x1
	mov cr0, eax

	jmp 0x08:modo_protegido

bits 32

modo_protegido:
	mov 	ax, 0x10
	mov 	ds, ax
	mov 	es, ax
	mov 	fs, ax
	mov 	ss, ax
	mov 	gs, ax
	mov 	esp, 0xA0000
	mov 	ebp, 0xA0000

	mov		eax, [mmap_entries]
	push	eax
	push	mmap_buffer

	call 	kernel_init
	jmp $


%define GDT_COUNT 6 ; WARNING, must match value defined in gdt.h

gdt_desc:
	dw (GDT_COUNT * 8) - 1
	dd gdt

; FIXME E820 function can overwrite code if it overflows this buffer.
%define MMAP_MAX_ENTRIES 10
%define MMAP_ENTRY_SIZE 24

mmap_buffer: times (MMAP_ENTRY_SIZE * MMAP_MAX_ENTRIES) db 0
