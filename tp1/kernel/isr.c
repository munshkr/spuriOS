#include <isr.h>
#include <i386.h>
#include <vga.h>
#include <debug.h>

// TODO? Rewrite everything in ASM and integrate with interrup.asm

isr_t interrupt_handlers[256] = {};

static inline void send_EOI(uint_8 irq);

void isr_handler(registers_t regs) {
	kassert(interrupt_handlers[regs.int_no] != NULL);

	isr_t handler = interrupt_handlers[regs.int_no];
	handler(regs);
}

void irq_handler(registers_t regs)
{
	// Send an EOI (end of interrupt) signal to the PICs
	send_EOI(regs.u.irq);

	kassert(interrupt_handlers[regs.int_no] != NULL);

	isr_t handler = interrupt_handlers[regs.int_no];
	handler(regs);
}

static inline void send_EOI(uint_8 irq)
{
	// If this interrupt involved the slave, send reset signal
	if (irq >= 8) {
		outb(0xA0, 0x20);
	}

	// Send reset signal to master
	outb(0x20, 0x20);
}
