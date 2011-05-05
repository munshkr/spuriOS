#ifndef __KBD_H__
#define __KBD_H__

#include <isr.h>

#define KBD_CTRL_PORT 0x64
#define KBD_DATA_PORT 0x60
#define kbd_scancode uint_8

void kbd_handler(registers_t* regs);
void kbd_init();

char sys_getch(void);
char get_scancode(void);

#endif // __KBD_H__
