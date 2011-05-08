;
; mmap.s ~ Memory map detection subroutines
;

;; ----------------------------------------
;; Relleno para entender el mapa de memoria
;
;  mov ecx, 50
;  mov di, 0x8000
;
;.relleno:
;  cmp ecx, 0
;  je .fin
;  mov dword [es:di], 0xFFFFFFFF
;  dec ecx
;  add di, 4
;  xchg bx, bx
;  jmp .relleno
;
;.fin:
;
;; ----------------------------------------
bits 16

extern _end
%define MMAP_ADDRESS _end

global mmap_entries
global make_mmap

mmap_entries dd  0x0000

make_mmap:
	pushad

	mov di, MMAP_ADDRESS	; Init
	xor ebx, ebx			;

	mov edx, 0x534d4150		;
	xor eax, eax			;
	mov eax, 0xe820			; INT_0x15 EAX_0xE820 Setup & Call
	mov ecx, 24				;
	int 0x15				;

	jc .failed				;
	mov edx, 0x534d4150		;
	cmp eax, edx			;
	jne .failed				; Error check
	cmp ebx, 0				;
	je .failed				;
	jmp .sucess				;

.ignore:
.loop:
	cmp ebx, 0				;
	je .end					; Check for finish

	mov edx, 0x534d4150		;
	xor eax, eax			;
	mov eax, 0xe820			; INT_0x15 EAX_0xE820 Setup & Call
	mov ecx, 24				;
	int 0x15				;

	jc .failed				;
	mov edx, 0x534d4150		;
	cmp eax, edx			; Status check
	jne .failed				;
	jmp .sucess				;

.failed:
	popad
	xor eax, eax
	ret

.sucess:
	mov ecx, [es:di + 8]	;
	or ecx, [es:di + 12]	; If the length of the entry is zero, ignore it
	jz .ignore				;

	add di, 24

	mov edx, [mmap_entries]	;
	inc edx					; Otherwise, increment and keep on maping
	mov [mmap_entries], edx	;

	jmp .loop

.end:
	popad
	mov eax, [mmap_entries]
	ret
