#include <i386.h>
#include <tipos.h>
#include <stdarg.h>
#include <common.h>
#include "vga.h"

uint_16 vga_port = 0x3D0;

uint_8* vga_addr = (uint_8*)0xB8000;
const uint_16 vga_cols = 80;
const uint_16 vga_rows = 25;

int vga_x = 0;
int vga_y = 0;
char vga_backcolor = VGA_BC_BLACK;
char vga_forecolor = VGA_FC_WHITE;


static void scroll(void);
static void putln(void);
static void update_cursor(void);

static unsigned int len(const int number, const char base);
static unsigned int ulen(const unsigned int number, const char base);
static int pow(const int base, const unsigned int exponent);
static int print_dec(const int number);
static int print_udec(const unsigned int number);
static int print_uhex(const unsigned int number);
static const char char_to_hex(const char val);


void vga_init(void) {

}

void vga_write(uint_16 f, uint_16 c, const char* msg, uint_8 attr) {
}

void vga_printf(uint_16 f, uint_16 c, const char* format, uint_8 attr, ...) {
}


void set_forecolor(const char color) {
	vga_forecolor = color;
}

void set_backcolor(const char color) {
	vga_backcolor = color;
}

void clear_colors(void) {
	vga_backcolor = VGA_BC_BLACK;
	vga_forecolor = VGA_FC_WHITE;
}

void clear(void) {
	// FIXME Use string's memset here
	short *pos = (short *) vga_addr;
	int i;
	for (i=0; i < vga_cols * vga_rows; ++i) {
		*pos++ = 0;
	}
	vga_x = 0;
	vga_y = 0;
	update_cursor();
}

int putchar(const char c) {
	if (c == '\n') {
		putln();
	} else if (c == '\t') {
		vga_x += TAB_WIDTH;
	} else {
		char attrib = vga_backcolor | vga_forecolor;
		volatile short *pos;
		pos = (short *) vga_addr + (vga_y * vga_cols + vga_x);
		*pos = c | (attrib << 8);
		vga_x++;
	}
	if (vga_x >= vga_cols) {
		putln();
	}
	return c;
}


/* Stripped down version of C standard `printf` with only these specifiers:
 *   %c (character)
 *   %s (string)
 *   %d (decimal integer)
 *   %u (unsigned decimal integer)
 *   %x (hexadecimal integer
 *   %% (write '%' character)
 */
int printf(const char* format, ...)
{
	int size = 0;
	va_list ap;
	const char* ptr = format;
	char* str;

	va_start(ap, format);
	while (*ptr) {
		if (*ptr == '%') {
			ptr++;
			switch (*ptr) {
			  case 'c':
				putchar((char) va_arg(ap, int));
				size++;
				break;
			  case 's':
				str = va_arg(ap, char*);
				while (*str) {
					putchar(*str++);
					size++;
				}
				break;
			  case 'd':
				size += print_dec(va_arg(ap, int));
				break;
			  case 'u':
				size += print_udec(va_arg(ap, unsigned int));
				break;
			  case 'x':
				size += print_uhex(va_arg(ap, int));
				break;
			  case '%':
				putchar('%');
				size++;
			}
		} else if (*ptr == '\\') {
			ptr++;
			switch (*ptr) {
				case 'c':
					ptr++;
					set_backcolor(char_to_hex(*ptr));
					ptr++;
					set_forecolor(char_to_hex(*ptr));
				break;
			}
		} else {
			putchar(*ptr);
			size++;
		}
		ptr++;
	}
	va_end(ap);
	clear_colors();

	return size;
}


static void scroll(void) {
	short *pos = (short *) vga_addr;
	short *cur_pos = pos + vga_cols;
	int row, col;
	for (row=1; row < vga_rows; ++row) {
		for (col=0; col < vga_cols; ++col) {
			*pos++ = *cur_pos++;
		}
	}
	for (col=0; col < vga_cols; ++col) {
		*pos++ = 0;
	}
}

static void putln(void) {
	vga_x = 0;
	vga_y++;
	if (vga_y == vga_rows) {
		scroll();
		vga_y--;
	}
	update_cursor();
}

static void update_cursor(void) {
	uint_16 location = vga_y * vga_cols + vga_x;

	// FIXME The base port (here assumed to be 0x3d4 and 0x3d5)
	// should be read from the BIOS data area.

	outb(0x3d4, 14);			// Send the high cursor byte
	outb(0x3d5, location >> 8);
	outb(0x3d4, 15);			// Send the low cursor byte
	outb(0x3d5, location);
}

// Get number of digits of a number (signed)
static unsigned int len(const int number, const char base) {
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
static unsigned int ulen(const unsigned int number, const char base) {
	unsigned int length = 1;
	unsigned int div = number;

	while (div) {
		div /= base;
		if (!div) break;
		length++;
	}
	return length;
}

// Exponentiation function
static int pow(const int base, const unsigned int exponent) {
	unsigned int i;
	unsigned int res = 1;

	for (i = 0; i < exponent; ++i) {
		res *= base;
	}
	return res;
}

static int print_dec(int number) {
	int size = 0;
	unsigned int i, digit;
	const unsigned int ln = len(number, 10);
	int mult = pow(10, ln - 1);

	if (number < 0) {
		putchar('-');
		number = -number;
		size++;
	}
	for (i = 0; i < ln; ++i) {
		digit = (number / mult) % 10;
		putchar((char) digit + ASCII_0);
		mult /= 10;
		size++;
	}
	return size;
}

static int print_udec(const unsigned int number) {
	unsigned int i, digit;
	const unsigned int ln = ulen(number, 10);
	int mult = pow(10, ln - 1);

	for (i = 0; i < ln; ++i) {
		digit = (number / mult) % 10;
		putchar((char) digit + ASCII_0);
		mult /= 10;
	}
	return ln;
}

static int print_uhex(const unsigned int number) {
	unsigned int i, digit;
	const unsigned int ln = ulen(number, 16);
	unsigned int mult = pow(16, ln - 1);

	putchar('0');
	putchar('x');
	for (i = 0; i < ln; ++i) {
		digit = (number / mult) % 16;
		if (digit < 10) {
			putchar((char) digit + ASCII_0);
		} else if (digit < 16) {
			putchar((char) (digit - 10) + ASCII_a);
		} else {
			putchar('?');
		}
		mult /= 16;
	}
	return ln + 2;
}

// FIXME devolver int
static const char char_to_hex(const char val) {
	int tmp = (unsigned int)val;
	if (tmp >= 48 && tmp <= 57) {
		tmp -= 48;
	} else if (tmp >= 97 && tmp <= 102) {
		tmp -= 87;
	} else if (tmp >= 65 && tmp <= 70) {
		tmp -= 55;
	}
	return (char)tmp;
}

