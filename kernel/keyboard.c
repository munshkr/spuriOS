static void keyboard_handler(registers_t regs) {
	uint_8 scancode = inb(0x60);
	vga_printf("Scancode: %u\n", scancode);
}

static void keyb_init() {
	idt_register(ISR_IRQ1, keyboard_handler, PL_KERNEL);

	outb(0x21,0xfd);
    outb(0xa1,0xff);
}
