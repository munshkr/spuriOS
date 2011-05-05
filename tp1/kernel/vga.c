#include <i386.h>
#include <tipos.h>
#include <common.h>
#include <debug.h>
#include "vga.h"

/* ASCII hex number for '0' and 'a' letters (for printing hex numbers) */
#define ASCII_0 0x30
#define ASCII_a 0x61

/* How many spaces a TAB char represents */
#define TAB_WIDTH 4

/* Space char for cursor display */
#define WHITE_SPACE 0x0720


uint_16 vga_port = 0x3D0;

uint_8* vga_addr = (uint_8*) 0xB8000;
const uint_16 vga_cols = 80;
const uint_16 vga_rows = 25;


/* Current cursor location and color attributes */
int vga_x = 0;
int vga_y = 0;
vga_attr_t vga_attr;


/* Auxiliary functions */
static void putchar(const char c, const char raw);
static void scroll(void);
static void putln(void);
static void update_cursor(void);
static unsigned int len(const int number, const char base);
static unsigned int ulen(const unsigned int number, const char base);
static int pow(const int base, const unsigned int exponent);
static int print_dec(const int number);
static int print_udec(const unsigned int number);
static int print_uhex(const unsigned int number);
static unsigned int scan_uhex(const char value);


void vga_init(void) {
	vga_attr.fld.forecolor = VGA_FC_WHITE;
	vga_attr.fld.backcolor = VGA_BC_BLACK;
	vga_clear();
}

void vga_clear(void) {
	memset(vga_addr, 0, vga_cols * vga_rows * 2);
	vga_reset_pos();
}

int vga_putchar(const char c) {
	putchar(c, TRUE);
	update_cursor();
	return c;
}

int vga_printf_fixed_args(const char* format, uint_32* args) {
	int size = 0;
	const char* ptr = format;
	char* str;

	// Save current attributes
	vga_attr_t old_attr = vga_attr;

	while (*ptr) {
		if (*ptr == '%') {
			ptr++;
			switch (*ptr) {
			  case 'c':
				putchar(*(char*) args, FALSE);
				args++;
				size++;
				break;
			  case 's':
				str = *(char**) args;
				args++;
				while (*str) {
					putchar(*str++, TRUE);
					size++;
				}
				break;
			  case 'd':
				size += print_dec(*(int*)args);
				args++;
				break;
			  case 'u':
				size += print_udec(*(unsigned int*)args);
				args++;
				break;
			  case 'x':
			  case 'p':
				size += print_uhex(*(int*)args);
				args++;
				break;
			  case '%':
				putchar('%', FALSE);
				size++;
			}
		} else if (*ptr == '\\') {
			ptr++;
			switch (*ptr) {
			  case 'c':
				ptr++;
				const char back = scan_uhex(*ptr);
				ptr++;
				const char fore = scan_uhex(*ptr);
				vga_attr.vl.vl = (back << 8) | fore;
			}
		} else {
			putchar(*ptr, FALSE);
			size++;
		}
		ptr++;
	}

	// Restore attributes
	vga_attr = old_attr;
	update_cursor();

	return size;
}

int vga_printf(const char* format, ...) {
	va_list ap;
	va_start(ap, format);
	int ret = vga_printf_fixed_args(format, (void*) ap);
	va_end(ap);
	return ret;
}

void vga_reset_colors(void) {
	vga_attr.vl.vl = VGA_BC_BLACK | VGA_FC_WHITE;
}

void vga_set_x(uint_16 x) {
	kassert(x < vga_cols);
	vga_x = x;
}

void vga_set_y(uint_16 y) {
	kassert(y < vga_rows);
	vga_y = y;
}

void vga_set_pos(uint_16 x, uint_16 y) {
	vga_set_x(x);
	vga_set_y(y);
}

void vga_reset_pos(void) {
	vga_x = 0;
	vga_y = 0;
	update_cursor();
}

int vga_get_x() {
	return vga_x;
}

int vga_get_y() {
	return vga_y;
}


static void putchar(const char c, const char raw) {
	if (c == '\n' && !raw) {
		putln();
	} else if (c == '\t' && !raw) {
		vga_x += TAB_WIDTH;
	} else {
		volatile short *pos;
		pos = (short *) vga_addr + (vga_y * vga_cols + vga_x);
		*pos = c | (vga_attr.vl.vl << 8);
		vga_x++;
	}
	if (vga_x >= vga_cols) {
		putln();
	}
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

// It Works, but cursor will only be visible if it's position has a character printed in.
// temp workaround: add a blank space before updating cursor.
// FIXME: we should find the right position in the code for the putchar(' ') sentence,
// because this way it's leaving a blank space each time we use putln() and if a backcolor
// is set, that blank space is visible. (check kernel.c)
static void update_cursor(void) {
	short location = vga_y * vga_cols + vga_x;
	volatile short *pos = (short *) vga_addr + location;
	*pos = WHITE_SPACE;

	outb(0x3D4, 0x0E);			// Send the high cursor byte
	outb(0x3D5, (unsigned char)((location >> 8) & 0xFF));
	outb(0x3D4, 0x0F);			// Send the low cursor byte
	outb(0x3D5, (unsigned char)(location & 0xFF));
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
		putchar('-', FALSE);
		number = -number;
		size++;
	}
	for (i = 0; i < ln; ++i) {
		digit = (number / mult) % 10;
		putchar((char) digit + ASCII_0, FALSE);
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
		putchar((char) digit + ASCII_0, FALSE);
		mult /= 10;
	}
	return ln;
}

static int print_uhex(const unsigned int number) {
	unsigned int i, digit;
	const unsigned int ln = ulen(number, 16);
	unsigned int mult = pow(16, ln - 1);

	putchar('0', FALSE);
	putchar('x', FALSE);

	for (i = 0; i < 8 - ln; i++) {
		putchar('0', FALSE);
	}

	for (i = 0; i < ln; ++i) {
		digit = (number / mult) % 16;
		if (digit < 10) {
			putchar((char) digit + ASCII_0, FALSE);
		} else if (digit < 16) {
			putchar((char) (digit - 10) + ASCII_a, FALSE);
		} else {
			putchar('?', FALSE);
		}
		mult /= 16;
	}
	return ln + 2;
}

static unsigned int scan_uhex(const char value) {
	unsigned int tmp = (unsigned int) value;
	if (tmp >= 48 && tmp <= 57) {
		tmp -= 48;
	} else if (tmp >= 97 && tmp <= 102) {
		tmp -= 87;
	} else if (tmp >= 65 && tmp <= 70) {
		tmp -= 55;
	}
	return tmp;
}
