#include <lib.h>
#include <common.h>

static sint_32 vsnprintf(char* buffer, sint_32 buff_size,
                         const char** format, uint_32* arg_ptr);

#ifndef __KERNEL__	// TASK-ONLY

#include <syscalls.h>
#include <errors.h>
#include <debug.h>

#define FPRINTF_BUFFER_SIZE 4096
char* __lib_fp_buffer = NULL;

// TODO Support fprintf with strings larger than FPRINTF_BUFFER_SIZE
sint_32 fprintf(fd_t file, const char* format, ...) {
	if (!*format) { return 0; }

	if (!__lib_fp_buffer) {
		__lib_fp_buffer = (char*) palloc();
		if (!__lib_fp_buffer) { return -ENOMEM; }
		memset(__lib_fp_buffer, 0, FPRINTF_BUFFER_SIZE);
	}

	va_list args;
	va_start(args, format);
	sint_32 sz_printf = vsnprintf(__lib_fp_buffer, FPRINTF_BUFFER_SIZE, &format, (uint_32*) args);
	va_end(args);

	sint_32 sz_write = write(file, __lib_fp_buffer, sz_printf);
	assert(sz_printf == sz_write);

	return sz_write;
}

// Semaphores
sint_32 sem_open(sem_t* sem) {
	return pipe(sem->pipes);
}

sint_32 sem_close(sem_t* sem) {
	sint_32 res = 0;
	res += close(sem->pipes[0]);
	res += close(sem->pipes[1]);
	return res;
}

sint_32 sem_signal(sem_t* sem, uint_32 count) {
	char buffer[count];
	return write(sem->pipes[1], buffer, count);
}

sint_32 sem_wait(sem_t* sem, uint_32 count) {
	char buffer[count];
	return read(sem->pipes[0], buffer, count);
}

char getch(fd_t fd) {
	char sc = 0;
	read(fd, &sc, 1);
	return sc;
}

#endif

sint_32 sprintf(char* buffer, const char* format, ...) {
	va_list args;
	va_start(args, format);

	int ret = vsnprintf(buffer, -1, &format, (uint_32*) args);

	va_end(args);
	return ret;
}

sint_32 snprintf(char* buffer, sint_32 buff_size, const char* format, ...) {
	va_list args;
	va_start(args, format);

	int ret = vsnprintf(buffer, buff_size, &format, (uint_32*) args);

	va_end(args);
	return ret;
}

sint_32 strcmp(const char* p, const char* q) {
	for (; *p && *q && *p==*q; p++,q++);
	return *p?(*q?(sint_32)*p-(sint_32)*q:1):*q?-1:0;
}

sint_32 strncmp(const char* p, const char* q, uint_32 n) {
	for (; n && *p && *q && *p==*q; p++,q++,n--);
	return n?(*p?(*q?(sint_32)*p-(sint_32)*q:1):*q?-1:0):0;
}

// Devuelve la próxima posición de character en string, empezando por from.
// OJO: cuando no encuentra el caracter devuelve FALSE, que es 0, con lo cual
// el valor de posición que devuelve esta función empieza en 1, no en 0.
uint_32 char_pos(const char* string, const char character, uint_32 from) {
	uint_32 i = from;
	bool found = FALSE;
	while(string[i] != 0) {
		if (string[i] == character) {
			found = TRUE;
			break;
		}
		i++;
	}
	if (found) {
		return ++i;
	} else {
		return 0;
	}
}

// Devuelve la posición del último caracter válido antes de un \0
// OJO: usar sólo cuando esten seguros de que hay un \0 en el string
// de modo contrario podés irte de tu memoria y morir por PF
uint_32 str_end_pos(const char* string, uint_32 from) {
	uint_32 i = from;
	while(string[i] != 0) {
		i++;
	}
	return i;
}

sint_32 pow(const sint_32 base, const uint_32 exponent) {
	uint_32 i;
	uint_32 res = 1;

	for (i = 0; i < exponent; ++i) {
		res *= base;
	}
	return res;
}

// Get number of digits of a number (signed)
uint_32 len(const sint_32 number, const char base) {
	uint_32 length = 1;
	uint_32 div = ABS(number);

	while (div) {
		div /= base;
		if (!div) break;
		length++;
	}
	return length;
}

// Get number of digits of a number (unsigned)
uint_32 ulen(const uint_32 number, const char base) {
	uint_32 length = 1;
	uint_32 div = number;

	while (div) {
		div /= base;
		if (!div) break;
		length++;
	}
	return length;
}

void memcpy(const void* src, void* dst, size_t size) {
	int i;
	for (i = 0; i < size; i++) {
		*(uint_8*)dst = *(uint_8*)src;
		src++;
		dst++;
	}
}

void memset(void* addr, int value, size_t size) {
	int i;
	for (i = 0; i < size; i++) {
		*(uint_8*)addr = value;
		addr++;
	}
}

static sint_32 put_dec(char** str, sint_32 number);
static sint_32 put_uhex(char** str, const uint_32 number);
static sint_32 print_udec(char** str, const uint_32 number);

static sint_32 vsnprintf(char* buffer, sint_32 buff_size,
                         const char** format, uint_32* arg_ptr)
{
	uint_32 sz;

	for (sz = 0; buff_size < 0 || sz < buff_size; ) {
		if (**format == '\0') {
			break;
		} else if (**format == '%') {
			(*format)++;
			switch (**format) {
			case 'c':
				*buffer = *(char*) arg_ptr;
				buffer++;
				arg_ptr++;
				sz++;
				break;
			case 's':;
				char* s = *(char**) arg_ptr;
				arg_ptr++;
				while ((buff_size < 0 || sz < buff_size) && *s) {
					*buffer = *s++;
					buffer++;
					sz++;
				}
				break;
			case 'd':
				// FIXME put_dec doesn't know if buffer is big enough to write!
				sz += put_dec(&buffer, *(sint_32*) arg_ptr);
				arg_ptr++;
				break;
			case 'u':
				// FIXME put_udec doesn't know if buffer is big enough to write!
				sz += print_udec(&buffer, *(uint_32*) arg_ptr);
				arg_ptr++;
				break;
			case 'x':
			case 'p':
				// FIXME put_uhex doesn't know if buffer is big enough to write!
				sz += put_uhex(&buffer, *(sint_32*) arg_ptr);
				arg_ptr++;
				break;
			case '%':
				*buffer = '%';
				buffer++;
				sz++;
				break;
			default:
				return -1;
			}
		} else {
			*buffer = **format;
			buffer++;
			sz++;
		}
		(*format)++;
	}

	*buffer = 0;
	return sz;
}

static sint_32 put_dec(char** str, sint_32 number) {
	sint_32 size = 0;
	uint_32 i, digit;
	const uint_32 ln = len(number, 10);
	sint_32 mult = pow(10, ln - 1);

	if (number < 0) {
		**str = '-'; (*str)++;
		number = -number;
		size++;
	}

	for (i = 0; i < ln; ++i) {
		digit = (number / mult) % 10;
		**str = (char) digit + ASCII_0; (*str)++;
		mult /= 10;
		size++;
	}

	return size;
}

static sint_32 put_uhex(char** str, const uint_32 number) {
	uint_32 i, digit;
	const uint_32 ln = ulen(number, 16);
	uint_32 mult = pow(16, ln - 1);

	**str = '0'; (*str)++;
	**str = 'x'; (*str)++;

	for (i = 0; i < 8 - ln; i++)
		{ **str = '0'; (*str)++; }

	for (i = 0; i < ln; ++i) {
		digit = (number / mult) % 16;
		if (digit < 10) {
			**str = (char) digit + ASCII_0; (*str)++;
		} else if (digit < 16) {
			**str = (char) (digit - 10) + ASCII_a; (*str)++;
		} else {
			**str = '?'; (*str)++;
		}
		mult /= 16;
	}
	return ln + 2;
}

static sint_32 print_udec(char** str, const uint_32 number) {
	uint_32 i, digit;
	const uint_32 ln = ulen(number, 10);
	sint_32 mult = pow(10, ln - 1);

	for (i = 0; i < ln; ++i) {
		digit = (number / mult) % 10;
		**str = (char) digit + ASCII_0; (*str)++;
		mult /= 10;
	}
	return ln;
}
