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

pid sc_queue = FREE_QUEUE; 

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

	loader_unqueue(&sc_queue);
}

char sys_getsc() {
	while (1) {	
		if (buffered_scancode != 0) {
			char tmp = buffered_scancode;
			buffered_scancode = KBD_SC_NONE;
			return tmp;
		} 
	
		loader_enqueue(&sc_queue);
	}
}
