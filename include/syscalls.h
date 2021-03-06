#ifndef __SYSCALLS_H__
#define __SYSCALLS_H__

#include <tipos.h>

#define SYS_INT 0x30

#define SYS_GETPID		0
#define SYS_EXIT		1
#define SYS_PALLOC		2
#define SYS_PRINT		3
#define SYS_SLEEP		5
#define SYS_LOCPRINT	6
#define SYS_RUN			7
#define SYS_FORK		8
#define SYS_PIPE		9
#define SYS_SHARE_PAGE  10

#define SYS_LOCPRINTALPHA	50

/* Device */
#define SYS_OPEN	30
#define SYS_CLOSE	31
#define SYS_READ	32
#define SYS_WRITE	33
#define SYS_SEEK	34

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

sint_32 run(const char* filename);
sint_32 fork();
sint_32 pipe(sint_32 pipes[2]);
sint_32 share_page(void* addr);

/* Deprecated */
int loc_printf(uint_32 row, uint_32 col, const char* format, ...);// __attribute__ ((format (printf, 3, 4)));
int loc_alpha_printf(uint_32 row, uint_32 col, const char* format, ...);// __attribute__ ((format (printf, 3, 4)));
int printf(const char* format, ...) __attribute__ ((format (printf, 1, 2)));

/* Device */
int open(const char* filename, uint_32 flags);
int close(int fd);
int read(int fd, void* buf, uint_32 size);
int write(int fd, const void* buf, uint_32 size);
int seek(int fd, uint_32 size);

#endif

#endif
