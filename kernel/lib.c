#include <lib.h>
#include <common.h>

static sint_32 vsnprintf(char* buffer, sint_32 buff_size,
                         const char** format, va_list arg_ptr);

#ifndef __KERNEL__	// TASK-ONLY

#include <syscalls.h>

char* __lib_fp_buffer = NULL;

sint_32 fprintf(fd_t file, const char* format, ...) {
	sint_32 size = 0;
	const char* str_p = format;
	while (*str_p) { str_p++; size++; }

	if (!size) { return size; }

	__lib_fp_buffer = (char*) (__lib_fp_buffer || (char*) palloc());
	if (!__lib_fp_buffer) { return 0; }

	va_list args;
	va_start(args, format);
	sint_32 ret = 0;
	while (ret < size) {
		ret += vsnprintf(__lib_fp_buffer, 4096, &format, args);
		write(file, __lib_fp_buffer, ret);
	}
	va_end(args);

	return ret;
}

#endif

sint_32 sprintf(char* buffer, const char* format, ...) {
	va_list args;
	va_start(args, format);

	int ret = vsnprintf(buffer, -1, &format, args);

	va_end(args);
	return ret;
}

sint_32 snprintf(char* buffer, sint_32 buff_size, const char* format, ...) {
	va_list args;
	va_start(args, format);

	int ret = vsnprintf(buffer, buff_size, &format, args);

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

static sint_32 put_dec(char** str, sint_32 number);
static sint_32 put_uhex(char** str, const uint_32 number);
static sint_32 print_udec(char** str, const uint_32 number);

static sint_32 vsnprintf(char* buffer, sint_32 buff_size,
                         const char** format, va_list arg_ptr)
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
				while (*s) { *buffer = *s++; buffer++; sz++; }
				break;
			case 'd':
				sz += put_dec(&buffer, *(sint_32*) arg_ptr);
				arg_ptr++;
				break;
			case 'u':
				sz += print_udec(&buffer, *(uint_32*) arg_ptr);
				arg_ptr++;
				break;
			case 'x':
			case 'p':
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