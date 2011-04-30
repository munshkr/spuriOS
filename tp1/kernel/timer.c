#include <timer.h>
#include <common.h>
#include <i386.h>
#include <idt.h>
#include <pic.h>
#include <vga.h>

uint_32 tick = 0;

static void int_timer(registers_t regs);

void timer_init(uint_32 frequency) {
	idt_register(ISR_IRQ0, int_timer, PL_KERNEL);

	// The value we send to the PIT is the value to divide it's input clock
	// (1193180 Hz) by, to get our required frequency. Important to note is
	// that the divisor must be small enough to fit into 16-bits.
	uint_32 divisor = 1193180 / frequency;

	// Send the command byte.
	outb(0x43, 0x36);

	// Send the frequency divisor.
	outb(0x40, (uint_8) (divisor & 0xFF));
	outb(0x40, (uint_8) ((divisor>>8) & 0xFF));

	// Enable IRQ0 in PICs
	pic_clear_irq_mask(0);
}

static void int_timer(registers_t regs) {
	tick++;
	vga_printf("Tick %u\n", tick);
}
