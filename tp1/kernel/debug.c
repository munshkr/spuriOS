#include <debug.h>
#include <isr.h>
#include <vga.h>
#include <mm.h>
#include <sched.h>
#include <i386.h>
#include <idt.h>
#include <common.h>

#define ST_EAX(x) *(x + 7)
#define ST_EBX(x) *(x + 4)
#define ST_ECX(x) *(x + 6)
#define ST_EDX(x) *(x + 5)
#define ST_ESI(x) *(x + 1)
#define ST_EDI(x) *(x + 0)
#define ST_EBP(x) *(x + 2)
#define ST_ESP(x) *(x + 3)

#define ST_ERR_CODE(x) *(x + 8)

#define SS_PL(x) (x & 3)

#define EXP_NUMBER(x) (x & ~0x80000000)
#define HAS_ERROR_CODE(x) (x & 0x80000000)

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
void debug_kernelpanic(uint_32 exp_number, const uint_32* expst) {
	/* No permite panics anidados */
	if (in_panic) while(1) hlt();
	in_panic = TRUE;

	vga_clear();
	vga_printf("\\c04Panic caused by [%s] ", exp_name[EXP_NUMBER(exp_number)]);

	uint_32 cs;
	uint_32* esp;
	if (HAS_ERROR_CODE(exp_number)) {
		cs = *(expst + 10);
		vga_printf("\\c04with error code %x (%d)\n", ST_ERR_CODE(expst), ST_ERR_CODE(expst));
		show_cs_eip(cs, *(expst + 9));
		show_eflags(*(expst + 11));

		if (SS_PL(cs) != PL_KERNEL) {
			esp = (uint_32*) *(expst + 12);
		} else {
			esp = (uint_32*) ST_ESP(expst);
		}

	} else {
		cs = *(expst + 9);
		vga_printf("\\c04with no error code\n");
		show_cs_eip(cs, *(expst + 8));
		show_eflags(*(expst + 10));
		
		if (SS_PL(cs) != PL_KERNEL) {
			esp = (uint_32*) *(expst + 11);
		} else {
			esp = (uint_32*) ST_ESP(expst);
		}
		show_stack(esp);
	}

	vga_printf("\nRegisters\n\tEAX = %x (%d), EBX = %x (%d), ECX = %x (%d)"\
		"\n\tEDX = %x (%d), ESI = %x (%d), EDI = %x (%d)\n",
		ST_EAX(expst), ST_EAX(expst), ST_EBX(expst), ST_EBX(expst), ST_ECX(expst), ST_ECX(expst),
		ST_EDX(expst), ST_EDX(expst), ST_ESI(expst), ST_ESI(expst), ST_EDI(expst), ST_EDI(expst));
}

void show_stack(uint_32* esp) {
	vga_printf("\nStack\n");

	char char_stack[17]; char_stack[16] = 0;

	uint_32 off;
	for (off = 0; off < 40; off += 4) {
		esp += off;
		memcpy(esp, &char_stack, 16);
		vga_printf("%p: %x %x %x %x %s\n", esp, *esp, *(esp + 1), *(esp + 2), *(esp + 3), char_stack);
	}

}

void show_cs_eip(uint_32 cs, uint_32 eip) {
	vga_printf("At (CS:EIP) %x:%x, privilege level %d\n", cs, eip, SS_PL(cs));
}

void show_eflags(uint_32 eflags) {
	vga_printf("Flags are %s%s%s%s%s%s%s%s%s(%x)\n",
		(eflags & 1    ? "CF " : "" ),
		(eflags & 4    ? "PF " : "" ),
		(eflags & 16   ? "AF " : "" ),
		(eflags & 64   ? "ZF " : "" ),
		(eflags & 128  ? "SF " : "" ),
		(eflags & 256  ? "TF " : "" ),
		(eflags & 512  ? "IF " : "" ),
		(eflags & 1024 ? "DF " : "" ),
		(eflags & 2048 ? "OF " : "" ),
		eflags );
}

void debug_init(void) {
	idt_register(0, isr_de, PL_KERNEL);
}

void debug_log(const char* message) {
	/* Perhaps this function should log to a file in the future */
	uint_64 tsc = read_tsc();
	vga_printf("[%d.%d] %s\n", (uint_32)(tsc >> 32), (uint_32)(tsc & 0xFFFFFFFF), message);
}
