#ifndef __SYSCALLS_H__
#define __SYSCALLS_H__

#include <tipos.h>

#define SYS_INT 0x30

#define SYS_GETPID   0
#define SYS_EXIT     1
#define SYS_PALLOC   2
#define SYS_PRINT    3
#define SYS_GETSC    4
#define SYS_SLEEP    5
#define SYS_LOCPRINT 6

#ifdef __KERNEL__

// Sólo se compila en modo "kernel"

void syscalls_init(void);

#else // __TAREA___

// Sólo se compila en modo "tarea"
// Declarar los "wrapers" para los syscalls que incluyen las tareas.

void sleep(uint_32 time);
uint_32 getpid(void);
void exit(void);
void* palloc(void);
int loc_printf(uint_32 row, uint_32 col, const char* format, ...);// __attribute__ ((format (printf, 3, 4)));
int printf(const char* format, ...) __attribute__ ((format (printf, 1, 2)));
//char getch(void);
char getsc(void);

#endif

#endif
