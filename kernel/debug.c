#include <debug.h>
#include <isr.h>
#include <vga.h>
#include <mm.h>
#include <sched.h>
#include <i386.h>
#include <idt.h>
#include <lib.h>

#define HAS_CF(x) (x & 1)
#define HAS_PF(x) (x & 4)
#define HAS_AF(x) (x & 16)
#define HAS_ZF(x) (x & 64)
#define HAS_SF(x) (x & 128)
#define HAS_TF(x) (x & 256)
#define HAS_IF(x) (x & 512)
#define HAS_DF(x) (x & 1024)
#define HAS_OF(x) (x & 2048)

typedef struct str_symbol_entry_t {
	uint_32 address;
	char* string;
} symbol_entry_t;

extern symbol_entry_t __symbols[];
extern uint_32 __symbols_total;

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

extern void spurious_master_handler(void);
extern void spurious_slave_handler(void);

void debug_init(void) {
	int i;
	for (i = 0; i < 20; i++) {
		idt_register(i, debug_kernelpanic, PL_KERNEL);
	}

	// FIXME This is not the correct way to handler spurious interrupts
	// because we're disabling handling of LPT1 interrupts or Secondary IDE.
	// We should read the ISR register of the PIC to find out if it's a fake
	// IRQ or a real one.  For now, we ignore them all.
	idt_register_asm(ISR_IRQ7, spurious_master_handler, PL_KERNEL);
	idt_register_asm(ISR_IRQ15, spurious_slave_handler, PL_KERNEL);
}

bool in_panic = FALSE;
void debug_kernelpanic(registers_t* regs) {
	/* No permite panics anidados */
	if (in_panic) while(1) hlt();
	in_panic = TRUE;

	vga_clear();
	vga_printf("\\c0CPanic caused by [%s] ", exp_name[regs->int_no]);

	if (regs->u.err_code != 0) {
		vga_printf("\\c0Cwith error code %x (%d)\n", regs->u.err_code, regs->u.err_code);
	} else {
		vga_printf("\\c0Cwith no error code\n");
	}

	if (regs->int_no == 0xE) { // #PF
		vga_printf("\\c0FAt vaddr %x, CR3 is %x, code = %s %s %s %s %s\n",
			rcr2(), rcr3(),
			(regs->u.err_code & 1 ? "P" : "N/P"),
			(regs->u.err_code & 2 ? "W" : "R"),
			(regs->u.err_code & 4 ? "U" : "S"),
			(regs->u.err_code & 8 ? "R" : "N/R"),
			(regs->u.err_code & 16? "F" : "N/F")
		);
	}

	show_cs_eip(regs->cs, regs->eip);
	show_eflags(regs->eflags);

	uint_32* esp;
	if (SS_PL(regs->cs) != PL_KERNEL) {
		esp = (uint_32*) regs->user_esp;
	} else {
		esp = (uint_32*) regs->esp;
	}
	show_stack(esp);
	show_backtrace((uint_32*) regs->ebp);

	vga_printf("\nRegisters\n  EAX = %x (%d), EBX = %x (%d), ECX = %x (%d)"\
		"\n  EDX = %x (%d), ESI = %x (%d), EDI = %x (%d)\n",
		regs->eax, regs->eax, regs->ebx, regs->ebx, regs->ecx, regs->ecx,
		regs->edx, regs->edx, regs->esi, regs->esi, regs->edi, regs->edi);

	// Die
	cli();
	hlt();
}

#define BT_MAX_PARAMS 2
#define BT_MAX 5
/* We assume calls were near and didn't push a segment selector */
void show_backtrace(uint_32* ebp) {
	vga_printf("\nBacktrace at %p\n", ebp);

	uint_32 param_id;
	uint_32* top = ebp;

	uint_32 back_count;
	for (back_count = 0; back_count < BT_MAX; back_count++) {
		if (*top < (uint_32) ebp) {
			break;
		}

		ebp = (uint_32*) *top;

		param_id = 0;
		top++;
		if (*top == 0 || *top == ~0) {
			vga_printf("\tInvalid return address (%x)\n", *top);
			break;
		}

		uint_8* from = (uint_8*)(((uint_8*)*top) - 5);
		uint_8* to = from + 5 + *(uint_32*)(from + 1);

		char *name = NULL;
		unsigned int displacement = 0;

		// Print `from` symbol name
		displacement = symbol_name((uint_32) from, &name);
		if (name != NULL) {
			if (displacement > 0) {
				vga_printf("\t[%s+%u]", name, displacement);
			} else {
				vga_printf("\t[%s]", name);
			}
		} else {
			vga_printf("\t%p", from);
		}

		// Print `to` symbol name
		displacement = symbol_name((uint_32) to, &name);
		if (name != NULL) {
			if (displacement > 0) {
				vga_printf(" -> [%s+%u] (", name, displacement);
			} else {
				vga_printf(" -> [%s] (", name);
			}
		} else {
			vga_printf(" -> %p (", to);
		}

		top++;
		if (top >= ebp) {
			vga_printf(")\n");
		}

		while (top < ebp) {
			if (param_id < BT_MAX_PARAMS && (top < ebp - 1)) {
				vga_printf("%x, ", *top);
			} else if (param_id == BT_MAX_PARAMS && (top < ebp - 1)) {
				vga_printf("%x...)\n", *top);
			} else if (param_id < BT_MAX_PARAMS && (top == ebp - 1)) {
				vga_printf("%x)\n", *top);
			}

			param_id++;
			top++;
		}
	}
}

void show_stack(uint_32* esp) {
	vga_printf("\nStack\n");

	char char_stack[17]; char_stack[16] = 0;

	uint_32 off;
	uint_32 i;
	for (off = 0; off < 0x1C; off += 4) {
		memcpy(esp, &char_stack, 16);
		vga_printf("%p: %x %x %x %x ", esp, *esp, *(esp + 1), *(esp + 2), *(esp + 3));

		for (i = 0; i < 16; i++) {
			vga_putchar(char_stack[i]);
		}

		vga_printf("\n");
		esp += 4;
	}

}

void show_cs_eip(uint_32 cs, uint_32 eip) {
	vga_printf("\\c0FAt (CS:EIP) %x:%x", cs, eip);

	char *name = NULL;
	unsigned int displacement = symbol_name(eip, &name);
	if (name != NULL) {
		if (displacement > 0) {
			vga_printf("\\c0F [%s+%u]", name, displacement);
		} else {
			vga_printf("\\c0F [%s]", name);
		}
	}

	vga_printf("\\c0F - privilege level %d\n", SS_PL(cs));
}

void show_eflags(uint_32 eflags) {
	vga_printf("Flags are %s%s%s%s%s%s%s%s%s(%x)\n",
		(HAS_CF(eflags) ? "CF " : "" ),
		(HAS_PF(eflags) ? "PF " : "" ),
		(HAS_AF(eflags) ? "AF " : "" ),
		(HAS_ZF(eflags) ? "ZF " : "" ),
		(HAS_SF(eflags) ? "SF " : "" ),
		(HAS_TF(eflags) ? "TF " : "" ),
		(HAS_IF(eflags) ? "IF " : "" ),
		(HAS_DF(eflags) ? "DF " : "" ),
		(HAS_OF(eflags) ? "OF " : "" ),
		eflags);
}

void debug_log(const char* message, ...) {
	/* Perhaps this function should log to a file in the future */
	uint_64 tsc = read_tsc();
	vga_printf("[%d.%d] ", (uint_32)(tsc >> 32), (uint_32)(tsc & 0xFFFFFFFF));
 	
	va_list ap;
	va_start(ap, message);
	vga_printf_fixed_args(message, (void*) ap);
	va_end(ap);
	vga_printf("\n");
}

unsigned int symbol_name(uint_32 address, char** string_p) {
	// Linear search on the symbols table
	int i;
	for (i = 0; i < __symbols_total; i++) {
		if (address >= __symbols[i].address &&
				address < __symbols[i + 1].address) {
			*string_p = __symbols[i].string;
			return address - __symbols[i].address;
		}
	}

	*string_p = NULL;
	return 0;
}
