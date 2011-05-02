global timer_handler

; == Scheduler ==
extern sched_tick

; == Loader ==
extern cur_pid
extern tmp_pid
extern processes

%define PCB_SZ 16
%define PCB_CR3_OFF 8
%define PCB_ESP_OFF 12

; == Debug ==
extern timer_draw_clock
extern tick

timer_handler:
	pushad

;	== Debug ==
;	xchg bx,bx
;	inc dword [tick]
;	call timer_draw_clock
;	==

	; Ask scheduler who is next
	call sched_tick

	cmp eax, [cur_pid]
	je go_back

	xchg bx,bx

	; Task switch needed
	mov [tmp_pid], eax

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
	; Restore CR3
	mov eax, [eax + PCB_CR3_OFF]
	mov cr3, eax

	; Set the new current process
	mov eax, [tmp_pid]
	mov [cur_pid], eax

go_back:
	; Send EOI
	mov al, 0x20
	out 0x20, al

	popad
	iret
