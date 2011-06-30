#include <i386.h>
#include <tipos.h>
#include <common.h>
#include <lib.h>
#include <debug.h>
#include "vga.h"

/* How many spaces a TAB char represents */
#define TAB_WIDTH 4

/* Space char for cursor display */
#define WHITE_SPACE 0x0720


uint_16 vga_port = 0x3D0;

uint_8* vga_addr = (uint_8*) 0xB8000;
const uint_16 vga_cols = 80;
const uint_16 vga_rows = 25;


/* Current cursor location and color attributes */
vga_screen_state_t vga_state;


/* Auxiliary functions */
static void scroll(const void* buff_ptr);
static void putchar(const void* buff_ptr, vga_screen_state_t* state,
                    const char c, const char raw);
static void putln(const void* buff_ptr, vga_screen_state_t* state);
static void putbs(const void* buff_ptr, vga_screen_state_t* state);

static int print_dec(const int number);
static int print_udec(const unsigned int number);
static int print_uhex(const unsigned int number);

static unsigned int scan_uhex(const char value);


void vga_init(void) {
	vga_clear();
	vga_state.attr.fld.forecolor = VGA_FC_WHITE;
	vga_state.attr.fld.backcolor = VGA_BC_BLACK;
}

void vga_clear(void) {
	memset(vga_addr, 0, vga_cols * vga_rows * 2);
	vga_reset_pos();
}

int vga_putchar(const char c) {
	putchar(vga_addr, &vga_state, c, TRUE);
	vga_update_cursor();
	return c;
}

int vga_writebuf(const void* buff_ptr, vga_screen_state_t* state,
                 const char* string, const size_t size)
{
	int sz;
	const char* ptr = string;

	vga_attr_t old_attr = state->attr;

	for (sz = 0; sz < size; ptr++, sz++) {
		if (*ptr == '\0') {
			sz++;
			break;
		} else if (*ptr == '\\') {
			ptr++;
			sz++;
			switch (*ptr) {
			  case 'c':
				ptr++;
				const char back = scan_uhex(*ptr);
				ptr++;
				const char fore = scan_uhex(*ptr);
				state->attr.vl.vl = (back << 8) | fore;
				sz+=2;
				break;
			  default:
				putchar(buff_ptr, state, '\\', FALSE);
				break;
			}
		} else {
			putchar(buff_ptr, state, *ptr, FALSE);
		}
	}

	state->attr = old_attr;

	return sz;
}

int vga_printf_fixed_args(const char* format, uint_32* args) {
	int size = 0;
	const char* ptr = format;
	char* str;

	// Save current attributes
	vga_attr_t old_attr = vga_state.attr;

	while (*ptr) {
		if (*ptr == '%') {
			ptr++;
			switch (*ptr) {
			  case 'c':
				putchar(vga_addr, &vga_state, *(char*) args, FALSE);
				args++;
				size++;
				break;
			  case 's':
				str = *(char**) args;
				args++;
				while (*str) {
					putchar(vga_addr, &vga_state, *str++, TRUE);
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
				putchar(vga_addr, &vga_state, '%', FALSE);
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
				vga_state.attr.vl.vl = (back << 4) | fore;
				break;
				default:
					putchar(vga_addr, &vga_state, '\\', FALSE);
					ptr--;
				break;
			}
		} else {
			putchar(vga_addr, &vga_state, *ptr, FALSE);
			size++;
		}
		ptr++;
	}

	// Restore attributes
	vga_state.attr = old_attr;
	vga_update_cursor();

	return size;
}


int vga_alpha_printf_fixed_args(const char* format, uint_32* args) {
	int size = 0;
	const char* ptr = format;
	char* str;

	// Save current attributes
	vga_attr_t old_attr = vga_state.attr;

	while (*ptr) {
		if (*ptr == '%') {
			ptr++;
			switch (*ptr) {
			  case 'c':
				putchar(vga_addr, &vga_state, *(char*) args, FALSE);
				args++;
				size++;
				break;
			  case 's':
				str = *(char**) args;
				args++;
				while (*str) {
					putchar(vga_addr, &vga_state, *str++, TRUE);
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
				putchar(vga_addr, &vga_state, '%', FALSE);
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
				vga_state.attr.vl.vl = (back << 4) | fore;
				break;
				default:
					putchar(vga_addr, &vga_state, '\\', FALSE);
					ptr--;
				break;
			}
		} else if (*ptr == ' ') {
			size++;
			vga_state.x++;
		} else {
			putchar(vga_addr, &vga_state, *ptr, FALSE);
			size++;
		}
		ptr++;
	}

	// Restore attributes
	vga_state.attr = old_attr;
	vga_update_cursor();

	return size;
}

int vga_printf(const char* format, ...) {
	va_list ap;
	va_start(ap, format);
	int ret = vga_printf_fixed_args(format, (void*) ap);
	va_end(ap);
	return ret;
}

int vga_loc_printf_fixed_args(uint_32 row, uint_32 col, const char* format, uint_32* args) {
	if (row > vga_rows || col > vga_cols) {
		return 0;
	}
	int old_x = vga_state.x;
	int old_y = vga_state.y;

	vga_state.x = col;
	vga_state.y = row;
	int ret = vga_printf_fixed_args(format, args);
	vga_state.x = old_x;
	vga_state.y = old_y;
	vga_update_cursor();

	return ret;
}

int vga_loc_alpha_printf_fixed_args(uint_32 row, uint_32 col, const char* format, uint_32* args) {
	if (row > vga_rows || col > vga_cols) {
		return 0;
	}
	int old_x = vga_state.x;
	int old_y = vga_state.y;

	vga_state.x = col;
	vga_state.y = row;
	int ret = vga_alpha_printf_fixed_args(format, args);
	vga_state.x = old_x;
	vga_state.y = old_y;
	vga_update_cursor();

	return ret;
}

void vga_reset_colors(void) {
	vga_state.attr.vl.vl = VGA_BC_BLACK | VGA_FC_WHITE;
}

void vga_reset_pos(void) {
	vga_state.x = 0;
	vga_state.y = 0;
	vga_update_cursor();
}

void vga_update_cursor(void) {
	short location = vga_state.y * vga_cols + vga_state.x;
	volatile short *pos = (short *) vga_addr + location;
	*pos = WHITE_SPACE;

	outb(0x3D4, 0x0E);			// Send the high cursor byte
	outb(0x3D5, (unsigned char)((location >> 8) & 0xFF));
	outb(0x3D4, 0x0F);			// Send the low cursor byte
	outb(0x3D5, (unsigned char)(location & 0xFF));
}


static void putchar(const void* buff_ptr, vga_screen_state_t* state,
                    const char c, const char raw)
{
	if (c == '\n' && !raw) {
		putln(buff_ptr, state);
	} else if (c == '\t' && !raw) {
		state->x += TAB_WIDTH;
	} else if (c == '\b' && !raw) {
		putbs(buff_ptr, state);
	} else {
		volatile short *pos;
		pos = (short *) buff_ptr + (state->y * vga_cols + state->x);
		*pos = c | (state->attr.vl.vl << 8);
		state->x++;
	}
	if (state->x >= vga_cols) {
		putln(buff_ptr, state);
	}
}

static void scroll(const void* buff_ptr) {
	// TODO Use memcpy and memset
	short *pos = (short *) buff_ptr;
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

static void putbs(const void* buff_ptr, vga_screen_state_t* state) {
	short location = state->y * vga_cols + state->x;
	volatile short *pos = (short *) buff_ptr + location;
	*pos = 0x20 | (state->attr.vl.vl << 8);

	if (state->x > 0) {
		state->x--;
	} else if (state->y > 0) {
		state->x = vga_cols - 1;
		state->y--;
	}
}

static void putln(const void* buff_ptr, vga_screen_state_t* state) {
	state->x = 0;
	state->y++;
	if (state->y == vga_rows) {
		scroll(buff_ptr);
		state->y--;
	}
	if (buff_ptr == vga_addr) {
		vga_update_cursor();
	}
}


static int print_dec(int number) {
	int size = 0;
	unsigned int i, digit;
	const unsigned int ln = len(number, 10);
	int mult = pow(10, ln - 1);

	if (number < 0) {
		putchar(vga_addr, &vga_state, '-', FALSE);
		number = -number;
		size++;
	}
	for (i = 0; i < ln; ++i) {
		digit = (number / mult) % 10;
		putchar(vga_addr, &vga_state, (char) digit + ASCII_0, FALSE);
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
		putchar(vga_addr, &vga_state, (char) digit + ASCII_0, FALSE);
		mult /= 10;
	}
	return ln;
}

static int print_uhex(const unsigned int number) {
	unsigned int i, digit;
	const unsigned int ln = ulen(number, 16);
	unsigned int mult = pow(16, ln - 1);

	putchar(vga_addr, &vga_state, '0', FALSE);
	putchar(vga_addr, &vga_state, 'x', FALSE);

	for (i = 0; i < 8 - ln; i++) {
		putchar(vga_addr, &vga_state, '0', FALSE);
	}

	for (i = 0; i < ln; ++i) {
		digit = (number / mult) % 16;
		if (digit < 10) {
			putchar(vga_addr, &vga_state, (char) digit + ASCII_0, FALSE);
		} else if (digit < 16) {
			putchar(vga_addr, &vga_state, (char) (digit - 10) + ASCII_a, FALSE);
		} else {
			putchar(vga_addr, &vga_state, '?', FALSE);
		}
		mult /= 16;
	}
	return ln + 2;
}

static unsigned int scan_uhex(const char value) {
	// TODO Define macros for the ascii characters used here
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

// Syscalls
void print(registers_t* regs) {
	// EBP (+0), EIP (+4), format (+8), ... args (+12)
	char* format = *(char**)(regs->ebp + 8);
	uint_32 ret = vga_printf_fixed_args(format, (uint_32*)(regs->ebp + 12));
	regs->eax = ret;
}

void loc_print(registers_t* regs) {
	//breakpoint();
	// EBP (+0), EIP (+4), row (+8), col (+12), format (+16), ... args (+20)
	uint_32 row = *(uint_32*)(regs->ebp + 8);
	uint_32 col = *(uint_32*)(regs->ebp + 12);
	char* format = *(char**)(regs->ebp + 16);
	uint_32 ret = vga_loc_printf_fixed_args(row, col, format, (uint_32*)(regs->ebp + 20));
	regs->eax = ret;
}

void loc_print_alpha(registers_t* regs) {
	//breakpoint();
	// EBP (+0), EIP (+4), row (+8), col (+12), format (+16), ... args (+20)
	uint_32 row = *(uint_32*)(regs->ebp + 8);
	uint_32 col = *(uint_32*)(regs->ebp + 12);
	char* format = *(char**)(regs->ebp + 16);
	uint_32 ret = vga_loc_alpha_printf_fixed_args(row, col, format, (uint_32*)(regs->ebp + 20));
	regs->eax = ret;
}
