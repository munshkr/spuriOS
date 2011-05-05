#ifndef __SYSCALLS_H__
#define __SYSCALLS_H__

#include <tipos.h>

#define SYS_INT 0x30

#define SYS_GETPID 0
#define SYS_EXIT   1
#define SYS_PALLOC 2
#define SYS_PRINT  3

#ifdef __KERNEL__

// Sólo se compila en modo "kernel"

void syscalls_init(void);

#else // __TAREA___

// Sólo se compila en modo "tarea"
// Declarar los "wrapers" para los syscalls que incluyen las tareas.

uint_32 getpid(void);
void exit(void);
void* palloc(void);
int printf(const char* format, ...) __attribute__ ((format (printf, 1, 2)));

#endif

#endif
