#include <lib.h>
#include <common.h>


sint_32 sprintf_fixed_args(char* str, const char* format, uint_32* args);
static sint_32 put_dec(char** str, sint_32 number);
static sint_32 put_uhex(char** str, const uint_32 number);
static sint_32 print_udec(char** str, const uint_32 number);


sint_32 sprintf(char* str, const char* format, ...) {
	va_list ap;
	va_start(ap, format);
	int ret = sprintf_fixed_args(str, format, (void*) ap);
	va_end(ap);
	return ret;
}

// FIXME Now that printf is not a syscall anymore, we don't need this workaround.
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
					size += put_dec(&str, *(sint_32*)args);
					args++;	break;
				case 'u':
					size += print_udec(&str, *(uint_32*)args);
					args++;	break;
				case 'x':
				case 'p':
					size += put_uhex(&str, *(sint_32*)args);
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
