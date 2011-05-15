#include <tipos.h>
#include <common.h>
#include <mm.h>
#include <loader.h>

int pow(const int base, const unsigned int exponent) {
	unsigned int i;
	unsigned int res = 1;

	for (i = 0; i < exponent; ++i) {
		res *= base;
	}
	return res;
}

// Get number of digits of a number (signed)
unsigned int len(const int number, const char base) {
	unsigned int length = 1;
	unsigned int div = ABS(number);

	while (div) {
		div /= base;
		if (!div) break;
		length++;
	}
	return length;
}

// Get number of digits of a number (unsigned)
unsigned int ulen(const unsigned int number, const char base) {
	unsigned int length = 1;
	unsigned int div = number;

	while (div) {
		div /= base;
		if (!div) break;
		length++;
	}
	return length;
}

sint_32 put_dec(char** str, sint_32 number) {
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

sint_32 put_uhex(char** str, const uint_32 number) {
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

sint_32 print_udec(char** str, const uint_32 number) {
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

sint_32 sprintf(char* str, const char* format, ...) {
	va_list ap;
	va_start(ap, format);
	int ret = sprintf_fixed_args(str, format, (void*) ap);
	va_end(ap);
	return ret;
}

sint_32 sprintf_fixed_args(char* str, const char* format, uint_32* args) {
	sint_32 size = 0;

	while (*format) {
		if (*format == '%') {
			format++;
			switch (*format) {
				case 'c':
					*str = *(char*) args; str++;
					args++;	size++;	break;
				case 's':;
					char* s = *(char**) args; args++;
					while (*s) { *str = *s++; str++; size++; }
					break;
				case 'd':
					size += put_dec(&str, *(int*)args);
					args++;	break;
				case 'u':
					size += print_udec(&str, *(unsigned int*)args);
					args++;	break;
				case 'x':
				case 'p':
					size += put_uhex(&str, *(int*)args);
					args++;	break;
				case '%':
					*str = '%'; str++;
					size++; break;
				default:
					return -1;
			}
		} else {
			*str = *format; str++;
			size++;
		}
		format++;
	}

	*str = 0;
	return size;
}

sint_32 copy2user(void* src, void* dst_usr, size_t size) {
	uint_32 pl = mm_pl_of_vaddr(dst_usr, cur_pdt());
	if (pl == PL_KERNEL) {
		return -1;
	} else {
		memcpy(src, dst_usr, size);
		return size;
	}
}

void memcpy(void* src, void* dst, size_t size) {
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

sint_32 strcmp(const char* p, const char* q) {
	for (; *p && *q && *p==*q; p++,q++);
	return *p?(*q?(sint_32)*p-(sint_32)*q:1):*q?-1:0;
}

sint_32 strncmp(const char* p, const char* q, uint_32 n) {
	for (; n && *p && *q && *p==*q; p++,q++,n--);
	return n?(*p?(*q?(sint_32)*p-(sint_32)*q:1):*q?-1:0):0;
}

