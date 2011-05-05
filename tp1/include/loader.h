#ifndef __LOADER_H__
#define __LOADER_H__

#include <pso_file.h>
#include <tss.h>
#include <syscalls.h>
#include <isr.h>

#define MAX_PID 32

#ifdef __KERNEL__

typedef struct str_pcb {
	pid id;
	uint_32 privilege_level;
	uint_32 cr3;
	uint_32 esp;
	pid prev, next;
} __attribute__((__packed__)) pcb_t;

extern pcb_t processes[];
extern pid cur_pid;

void loader_init(void);
pid loader_load(pso_file* f, uint_32 pl);

void loader_enqueue(pid* cola);
void loader_unqueue(pid* cola);

void loader_exit(void);

#endif

/* Syscalls */
// pid getpid(void);
// void exit(pid pd);

#endif
