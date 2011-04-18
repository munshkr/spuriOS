#include <debug.h>
#include <isr.h>
#include <vga.h>
#include <mm.h>
#include <sched.h>
#include <i386.h>
#include <idt.h>
#include <common.h>

const char* exp_name[] = {
	"Divide Error",
	"Debug Interrupt",
	"NMI Interrupt",
	"Breakpoint",
	"Interrupt on overflow",
	"BOUND range exceeded",
	"Invalid Opcode",
	"Device not available",
	"Double fault",
	"Coprocessor Segment Overrun",
	"Invalid TSS",
	"Segment not present",
	"Stack exception",
	"General protection fault",
	"Page fault",
	"Reserved",
	"Floating point exception",
	"Alignment check"
};

bool in_panic = FALSE;
void debug_kernelpanic(const uint_32* stack, const exp_state* expst) {
	/* No permite panics anidados */
	if (in_panic) while(1) hlt();
	in_panic = TRUE;

	// Completar
	debug_log("panic attack!");
}


void debug_init(void) {
	idt_register(0, isr_panic, PL_KERNEL);
}

void debug_log(const char* message) {
	/* Perhaps this function should log to a file in the future */
	uint_64 tsc = read_tsc();
	vga_printf("[%d.%d] %s\n", (uint_32)(tsc >> 32), (uint_32)(tsc & 0xFFFFFFFF), message);
}
