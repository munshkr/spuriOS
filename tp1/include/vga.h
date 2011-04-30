#ifndef __VGA_H__
#define __VGA_H__

#include <tipos.h>

typedef union str_vga_attr_t {
	struct str_vga_attr_fld {
		unsigned forecolor:3;
		signed light:1;
		unsigned backcolor:3;
		signed blink:1;
	} fld;
	struct str_vga_attr_vl {
		unsigned char vl;
	} vl;
} __attribute__((__packed__)) vga_attr_t;


extern uint_8* vga_addr;
extern const uint_16 vga_cols;
extern const uint_16 vga_rows;
extern vga_attr_t vga_attr;


void vga_init(void);

/* Original prototypes (deprecated) */
/*
void vga_write(uint_16 f, uint_16 c, const char* msg, uint_8 attr);
void vga_printf(uint_16 f, uint_16 c, const char* format, uint_8 attr, ...)
	__attribute__ ((format (printf, 1, 2)));
*/


/* Clear screen */
void vga_clear(void);

/* Put a char at current location */
int vga_putchar(const char c);

/* Print a string at current location
 *
 * Supported specifiers:
 *   %c   (character)
 *   %s   (string)
 *   %d   (decimal integer)
 *   %u   (unsigned decimal integer)
 *   %x   (hexadecimal integer
 *   %%   (write '%' character)
 *   \\cNN (set color attribute, where NN is attribute byte)
 */
int vga_printf(const char* format, ...) __attribute__ ((format (printf, 1, 2)));

/* Set foreground and background colors */
void vga_reset_colors(void);

/* Set cursor location */
void vga_set_x(uint_16 x);
int vga_get_x(void);
void vga_set_y(uint_16 y);
int vga_get_y(void);
void vga_set_pos(uint_16 x, uint_16 y);
void vga_reset_pos(void);


/* Color attributes */
#define VGA_FC_BLACK   0x00
#define VGA_FC_BLUE    0x01
#define VGA_FC_GREEN   0x02
#define VGA_FC_CYAN    0x03
#define VGA_FC_RED     0x04
#define VGA_FC_MAGENTA 0x05
#define VGA_FC_BROWN   0x06
#define VGA_FC_WHITE   0x07

#define VGA_FC_LIGHT   0x08
#define VGA_FC_BLINK   0x80

#define VGA_BC_BLACK   0x00
#define VGA_BC_BLUE    0x10
#define VGA_BC_GREEN   0x20
#define VGA_BC_CYAN    0x30
#define VGA_BC_RED     0x40
#define VGA_BC_MAGENTA 0x50
#define VGA_BC_BROWN   0x60
#define VGA_BC_WHITE   0x70


#endif
