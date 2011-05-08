#include <debug.h>
#include <isr.h>
#include <vga.h>
#include <mm.h>
#include <sched.h>
#include <i386.h>
#include <idt.h>
#include <common.h>

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

extern void spurious_handler(void);


void debug_init(void) {
	int i;
	for (i = 0; i < 20; i++) {
		idt_register(i, debug_kernelpanic, PL_KERNEL);
	}

	// FIXME This is not the correct way to handler spurious interrupts
	// because we're disabling handling of LPT1 interrupts or Secondary IDE.
	// We should read the ISR register of the PIC to find out if it's a fake
	// IRQ or a real one.  For now, we ignore them all.
	idt_register_asm(ISR_IRQ7, spurious_handler, PL_KERNEL);
	idt_register_asm(ISR_IRQ15, spurious_handler, PL_KERNEL);
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

	vga_printf("\nRegisters\n\tEAX = %x (%d), EBX = %x (%d), ECX = %x (%d)"\
		"\n\tEDX = %x (%d), ESI = %x (%d), EDI = %x (%d)\n",
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

		uint_8* call_addr = (uint_8*)(((uint_8*)*top) - 5);
		vga_printf("\tFrom %p, call %p (", call_addr, call_addr + 5 + *(uint_32*)(call_addr + 1));

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

void debug_log(const char* message) {
	/* Perhaps this function should log to a file in the future */
	uint_64 tsc = read_tsc();
	vga_printf("[%d.%d] %s\n", (uint_32)(tsc >> 32), (uint_32)(tsc & 0xFFFFFFFF), message);
}

unsigned int symbol_name(uint_32 address, char** string_p) {
	// Linear search on the symbols table
	int i;
	for (i = 0; i < __symbols_total; i++) {
		if (address >= __symbols[i].address &&
				address < __symbols[i + 1].address) {
			*string_p = __symbols[i].string;
			return __symbols[i + 1].address - address;
		}
	}

	*string_p = NULL;
	return 0;
}
