#ifndef __LOADER_H__
#define __LOADER_H__

#include <pso_file.h>
#include <tss.h>
#include <syscalls.h>
#include <isr.h>
#include <mm.h>

// run() syscall error codes
#define RUN_ERROR_READING		1
#define RUN_INVALID_EXECUTABLE	2
#define RUN_ERROR_OPENING		3
#define RUN_UNAVAILABLE_MEMORY	4

#ifdef __KERNEL__

#define FREE_PCB_PID 0xFFFFFFFF
#define FREE_QUEUE 0xFFFFFFFF
#define USER_MEMORY_START 0x400000

#define MAX_PID 32

typedef struct str_pcb {
	pid id;
	uint_32 privilege_level;
	uint_32 cr3;
	uint_32 esp;
	uint_32 next_empty_page_addr;
	pid prev, next;
} __attribute__((__packed__)) pcb_t;

typedef struct str_slept_task {
	pid id;
	uint_32 time;
	pid next;
} slept_task;

extern pcb_t processes[];
extern pid cur_pid;

mm_page* cur_pdt();

void loader_init(void);
pid loader_load(pso_file* f, uint_32 pl);

void loader_enqueue(pid* cola);
void loader_unqueue(pid* cola);

void loader_sleep(uint_32 time);
void loader_tick();
void loader_print_raw_sleeping();
void loader_print_sleeping();

void loader_exit(void);

/* Syscalls */
pid fork();
pid run(const char* filename);
pid getpid(void);
void exit(void);
void sleep(registers_t* regs);

#endif // __KERNEL__

#endif
