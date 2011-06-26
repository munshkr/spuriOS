#ifndef __LIB_H__
#define __LIB_H__

#include <tipos.h>

/* ASCII hex number for '0' and 'a' letters (for printing hex numbers) */
#define ASCII_0 0x30
#define ASCII_a 0x61

sint_32 sprintf(char* buffer, const char* format, ...);
sint_32 snprintf(char* buffer, sint_32 buff_size, const char* format, ...);

sint_32 strcmp(const char* p, const char* q);
sint_32 strncmp(const char* p, const char* q, uint_32 n);
uint_32 char_pos(const char* string, const char character, uint_32 from);
uint_32 str_end_pos(const char* string, uint_32 from);

sint_32 pow(const sint_32 base, const uint_32 exponent);
uint_32 len(const sint_32 number, const char base);
uint_32 ulen(const uint_32 number, const char base);

void memcpy(const void* src, void* dst, size_t size);
void memset(void* addr, int value, size_t size);


#ifndef __KERNEL__	// TASK-ONLY

sint_32 fprintf(fd_t file, const char* format, ...);

typedef struct str_sem_t {
	sint_32 pipes[2];
} sem_t;

sint_32 sem_open(sem_t* sem);
sint_32 sem_close(sem_t* sem);
sint_32 sem_signal(sem_t* sem, uint_32 count);
sint_32 sem_wait(sem_t* sem, uint_32 count);

#endif

#endif // __LIB_H__
