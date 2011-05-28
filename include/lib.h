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

sint_32 pow(const sint_32 base, const uint_32 exponent);
uint_32 len(const sint_32 number, const char base);
uint_32 ulen(const uint_32 number, const char base);

#ifndef __KERNEL__	// TASK-ONLY

sint_32 fprintf(fd_t file, const char* format, ...);

#endif

#endif // __LIB_H__
