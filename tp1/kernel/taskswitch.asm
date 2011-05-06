global timer_handler
global task_init
global task_switch

; == Scheduler ==
extern sched_tick

; == Loader ==
extern cur_pid
extern tmp_pid
extern processes
extern the_tss
extern loader_tick

%define TSS_ESP0_OFF 4

%define PCB_SZ 24
%define PCB_PL_OFF 4
%define PCB_CR3_OFF 8
%define PCB_ESP_OFF 12

%define PL_KERNEL 0
%define PL_USER 3
%define SS_U_DATA 0x23
%define SS_K_DATA 0x10

%define K_STACK_TOP 0xffc00000

; == Debug ==
extern timer_draw_clock
extern tick

timer_handler:
	pushad

	; Send EOI
	mov al, 0x20
	out 0x20, al

	inc dword [tick]
	call loader_tick

	;== Debug ==
	;xchg bx,bx
	;call timer_draw_clock
	;==

	; Ask scheduler who is next
	call sched_tick

	cmp eax, [cur_pid]
	je go_back

	;xchg bx,bx

	; Task switch needed
	mov [tmp_pid], eax

	call task_switch
go_back:
	popad
	iret

task_switch:
	pushad

	; Save current process information
	mov eax, [cur_pid]
	mov ebx, PCB_SZ
	mul ebx
	add eax, processes

	; Save stack pointer
	mov [eax + PCB_ESP_OFF], esp

	; Restore destination process information
	mov eax, [tmp_pid]
	mov ebx, PCB_SZ
	mul ebx
	add eax, processes

	; Restore stack pointer
	mov esp, [eax + PCB_ESP_OFF]

	; Set data segments
	mov esi, eax
	mov ebx, [eax + PCB_PL_OFF]
	cmp ebx, PL_KERNEL
	je set_kernel_segments

	; User data segments
	mov ax, SS_U_DATA
	mov ds, ax
	mov es, ax

	mov dword [the_tss + TSS_ESP0_OFF], K_STACK_TOP

	jmp restore_cr3
set_kernel_segments:
	; Kernel data segments
	mov ax, SS_K_DATA
	mov ds, ax
	mov es, ax

	mov dword [the_tss + TSS_ESP0_OFF], 0 ; Never used

restore_cr3:
	; Restore CR3
	mov eax, [esi + PCB_CR3_OFF]
	mov cr3, eax

	; Set the new current process
	mov eax, [tmp_pid]
	mov [cur_pid], eax

	popad
	ret

task_init:
	iret
