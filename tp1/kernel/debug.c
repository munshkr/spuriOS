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

void debug_init(void) {
	idt_register(0, isr_de, PL_KERNEL);
	idt_register(1, isr_db, PL_KERNEL);
	idt_register(2, isr_nmi, PL_KERNEL);
	idt_register(3, isr_bp, PL_KERNEL);
	idt_register(4, isr_of, PL_KERNEL);
	idt_register(5, isr_br, PL_KERNEL);
	idt_register(6, isr_ud, PL_KERNEL);
	idt_register(7, isr_nm, PL_KERNEL);
	idt_register(8, isr_df, PL_KERNEL);
	idt_register(9, isr_cso, PL_KERNEL);
	idt_register(10, isr_ts, PL_KERNEL);
	idt_register(11, isr_np, PL_KERNEL);
	idt_register(12, isr_ss, PL_KERNEL);
	idt_register(13, isr_gp, PL_KERNEL);
	idt_register(14, isr_pf, PL_KERNEL);
	idt_register(16, isr_mf, PL_KERNEL);
	idt_register(17, isr_ac, PL_KERNEL);
	idt_register(18, isr_mc, PL_KERNEL);
	idt_register(19, isr_xm, PL_KERNEL);
}

bool in_panic = FALSE;
void debug_kernelpanic(uint_32 exp_number, const uint_32* expst) {
	/* No permite panics anidados */
	if (in_panic) while(1) hlt();
	in_panic = TRUE;

	vga_clear();
	vga_printf("\\c0CPanic caused by [%s] ", exp_name[EXP_NUMBER(exp_number)]);

	uint_32 cs;
	uint_32* esp;
	if (HAS_ERROR_CODE(exp_number)) {
		cs = *(expst + 10);
		vga_printf("\\c0Cwith error code %x (%d)\n", ST_ERR_CODE(expst), ST_ERR_CODE(expst));
		show_cs_eip(cs, *(expst + 9));
		show_eflags(*(expst + 11));

		if (SS_PL(cs) != PL_KERNEL) {
			esp = (uint_32*) *(expst + 12);
		} else {
			esp = (uint_32*) ST_ESP(expst);
		}
		show_stack(esp);
		show_backtrace((uint_32*) ST_EBP(expst));

	} else {
		cs = *(expst + 9);
		vga_printf("\\c0Cwith no error code\n");
		show_cs_eip(cs, *(expst + 8));
		show_eflags(*(expst + 10));
		
		if (SS_PL(cs) != PL_KERNEL) {
			esp = (uint_32*) *(expst + 11);
		} else {
			esp = (uint_32*) ST_ESP(expst);
		}
		show_stack(esp);
		show_backtrace((uint_32*) ST_EBP(expst));
	}

	vga_printf("\nRegisters\n\tEAX = %x (%d), EBX = %x (%d), ECX = %x (%d)"\
		"\n\tEDX = %x (%d), ESI = %x (%d), EDI = %x (%d)\n",
		ST_EAX(expst), ST_EAX(expst), ST_EBX(expst), ST_EBX(expst), ST_ECX(expst), ST_ECX(expst),
		ST_EDX(expst), ST_EDX(expst), ST_ESI(expst), ST_ESI(expst), ST_EDI(expst), ST_EDI(expst));
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
	for (off = 0; off < 0x20; off += 4) {
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
	vga_printf("\\c0FAt (CS:EIP) %x:%x, privilege level %d\n", cs, eip, SS_PL(cs));
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

void debug_log(const char* message) {
	/* Perhaps this function should log to a file in the future */
	uint_64 tsc = read_tsc();
	vga_printf("[%d.%d] %s\n", (uint_32)(tsc >> 32), (uint_32)(tsc & 0xFFFFFFFF), message);
}
