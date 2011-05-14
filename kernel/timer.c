#include <timer.h>
#include <common.h>
#include <i386.h>
#include <idt.h>
#include <pic.h>
#include <vga.h>
#include <debug.h>

uint_32 tick = 0;
const char clock_anim[4] = {'-', '\\', '|', '/'};

void timer_init(uint_32 frequency) {
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

void timer_draw_clock() {
	int old_x = vga_get_x();
	int old_y = vga_get_y();

	// Draw a clock in the lower right corner of the screen
	vga_set_pos(vga_cols - 2, vga_rows - 1);
	vga_printf("\\c09%c", clock_anim[tick % 4]);

	// Restore cursor position
	vga_set_pos(old_x, old_y);
}

void timer_digital_clock() {
	uint_32 time = tick / 100;
	int old_x = vga_get_x();
	int old_y = vga_get_y();

	// Draw a clock in the lower right corner of the screen
	vga_set_pos(vga_cols - ulen(time/10, 10) - 4, 0);
	vga_printf("\\c0B%u.%us", (time / 10), (time % 10));

	// Restore cursor position
	vga_set_pos(old_x, old_y);
}
