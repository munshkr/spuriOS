#include <kbd.h>
#include <vga.h>
#include <pic.h>
#include <debug.h>
#include <i386.h>
#include <idt.h>
#include <common.h>
#include <loader.h>

#define KBD_SC_NONE 0
#define KBD_SC_SHIFT_PRESS   0x2A
#define KBD_SC_SHIFT_RELEASE (KBD_SC_SHIFT_PRESS + 0x80)

#define KBD_CTRL_NONE  0
#define KBD_CTRL_SHIFT 1

pid char_queue = FREE_QUEUE; 

char lowercase[256] = { [0x1E] ='a'};
char uppercase[256] = { [0x1E] ='A'};

char buffered_char = 0;
kbd_scancode buffered_scancode = KBD_SC_NONE;
uint_8 buffered_ctrl = KBD_CTRL_NONE;

void kbd_init() {
	debug_log("setting up keyboard");
	pic_clear_irq_mask(1);
	idt_register(ISR_IRQ1, kbd_handler, PL_KERNEL); 
}

inline void wait_byte() {
	uint_8 ctrl;
	for (ctrl = 0; (ctrl & 1) == 0; ctrl = inb(KBD_CTRL_PORT));
}

inline kbd_scancode read_scancode() {
	return inb(KBD_DATA_PORT);
}

void kbd_handler(registers_t* regs) {
	wait_byte();
	buffered_scancode = read_scancode();

	switch (buffered_scancode) {
		case KBD_SC_SHIFT_PRESS:
			buffered_ctrl |= KBD_CTRL_SHIFT; break;
		case KBD_SC_SHIFT_RELEASE:
			buffered_ctrl &= ~KBD_CTRL_SHIFT; break; 
		default:
			if (buffered_scancode & 0x80) {
				/* Ignore the break code */
			} else {
				buffered_char = (buffered_ctrl & KBD_CTRL_SHIFT ?
					uppercase : lowercase)[buffered_scancode];
			}
			break;
	}

	vga_printf("IRQ1 %x %d\n", buffered_scancode, buffered_ctrl);
}


// TODO The following functions will be syscalls for user tasks

char sys_getch() {
	
	if (buffered_char != 0) {
		char tmp = buffered_char;
		buffered_char = 0;
		return tmp;
	} 

	loader_enqueue(&char_queue);

	return 0;
}

char get_scancode() {
	// wait for break code and return
	while (buffered_scancode == KBD_SC_NONE) { };
	return buffered_scancode;
}
