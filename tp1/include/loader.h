#ifndef __LOADER_H__
#define __LOADER_H__

#include <pso_file.h>
#include <tss.h>
#include <syscalls.h>
#include <isr.h>

#define PID_IDLE_TASK 0
#define MAX_PID 32

#ifdef __KERNEL__

typedef struct str_arch_state {
	uint_32 eax, ebx, ecx, edx, esi, edi, ebp, esp, eflags, eip;
	uint_32 cr3;
	uint_16 code_segment, data_segment;
} arch_state_t;

typedef struct str_pcb {
	pid id;
	uint_32 privilege_level;	
	arch_state_t arch_state;
} pcb_t;

extern pcb_t processes[];
extern pid cur_pid;

void loader_init(void);
pid loader_load(pso_file* f, int pl);

void loader_enqueue(int* cola);
void loader_unqueue(int* cola);

void loader_exit(void);

void loader_switchto(pid pd, registers_t* regs);

#endif

/* Syscalls */
// pid getpid(void);
// void exit(pid pd);

#endif
