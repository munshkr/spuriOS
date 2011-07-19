global ap_trampoline

extern gdt
extern enable_paging
extern processors
extern processor_ap_kinit

%define PROCESSOR_T_SZ			134
%define PROCESSOR_LAPIC_ID_OFF	16
%define PROCESSOR_PRESENT_OFF	0
%define PROCESSOR_STACK_OFF		8
%define MAX_PROCESSORS			4

ALIGN 4096
bits 16
ap_trampoline:
	lgdt [gdt_desc]

	mov eax, cr0
	or eax, 0x1
	mov cr0, eax

	jmp 0x08:protected_mode

bits 32
protected_mode:
	mov 	ax, 0x10
	mov 	ds, ax
	mov 	es, ax
	mov 	fs, ax
	mov 	ss, ax
	mov 	gs, ax

	; Get local APIC ID (Pentium, P6, P4, Xeon)
	mov		eax, 1
	cpuid
	shr		ebx, 24

	; Search for this processor entry
	mov		esi, processors
	mov		ecx, MAX_PROCESSORS
_processor:
	cmp		dword [esi + PROCESSOR_PRESENT_OFF], 0
	je		_next_processor
	cmp		dword [esi + PROCESSOR_LAPIC_ID_OFF], ebx
	je		_found_processor

_next_processor:
	add esi, PROCESSOR_T_SZ
	loop _processor

_found_processor:
	mov		esp, [esi + PROCESSOR_STACK_OFF]
	add		esp, 0x1000
	mov		ebp, esp

	cld		; Needed by GCC (?)
	push	esi
	call	processor_ap_kinit

_hlt:
	hlt
	jmp _hlt

%define GDT_COUNT 20 ; WARNING, must match value defined in gdt.h

gdt_desc:
	dw (GDT_COUNT * 8) - 1
	dd gdt

