#include <tipos.h>
#include <i386.h>
#include <gdt.h>
#include <isr.h>
#include <idt.h>
#include <pic.h>
#include <common.h>
#include <debug.h>

/* Defined in isr.c */
extern isr_t interrupt_handlers[];

/* Defined in interrup.asm */
extern void (*idt_de)(void);
extern void (*idt_db)(void);
extern void (*idt_nmi)(void);
extern void (*idt_bp)(void);
extern void (*idt_of)(void);
extern void (*idt_br)(void);
extern void (*idt_ud)(void);
extern void (*idt_nm)(void);
extern void (*idt_df)(void);
extern void (*idt_cso)(void);
extern void (*idt_ts)(void);
extern void (*idt_np)(void);
extern void (*idt_ss)(void);
extern void (*idt_gp)(void);
extern void (*idt_pf)(void);
extern void (*idt_mf)(void);
extern void (*idt_ac)(void);
extern void (*idt_mc)(void);
extern void (*idt_xm)(void);

static const uint_32 IDT_ATTR_DPL_[4] = { IDT_ATTR_DPL0, IDT_ATTR_DPL1, IDT_ATTR_DPL2, IDT_ATTR_DPL3 };

/* Macro para crear una entrada de la IDT dando offset(32), selector(16) y attr(16). */
#define make_idt_entry(offset, select, attr) \
	(idt_entry){{((uint_32)(offset) & 0xFFFF) | ((uint_32)(select) << 16), \
	((uint_32)(attr) & 0xFFFF) | ((uint_32)(offset) & 0xFFFF0000) }}

#define idt_entry_null make_idt_entry(0,0,0)

#define IDT_INT IDT_ATTR_P | IDT_ATTR_S_ON | IDT_ATTR_D_32 | IDT_ATTR_TYPE_INT
#define IDT_EXP IDT_ATTR_P | IDT_ATTR_S_ON | IDT_ATTR_D_32 | IDT_ATTR_TYPE_EXP

#define IDT_ENTRIES 256

#define PIC1_START_IRQ 0x20
#define PIC2_START_IRQ 0x28

#define SS_K_CODE 0x8
#define SS_U_CODE 0x18

idt_entry idt[IDT_ENTRIES] = {};

idt_descriptor IDT_DESC = {sizeof(idt)-1, (uint_32)&idt};

/* Array of pointers to the IDT handlers */
void (*idt_handlers[])(void) = {};


void idt_init(void) {
	debug_log("initializing interrupt handling");

	idt_handlers[ISR_DIVIDE] = idt_de;
	idt_handlers[ISR_DEBUG] = idt_db;
	idt_handlers[ISR_NMI] = idt_nmi;
	idt_handlers[ISR_BRKPT] = idt_bp;
	idt_handlers[ISR_OFLOW] = idt_of;
	idt_handlers[ISR_BOUND] = idt_br;
	idt_handlers[ISR_ILLOP] = idt_ud;
	idt_handlers[ISR_DEVICE] = idt_nm;
	idt_handlers[ISR_DBLFLT] = idt_df;
	idt_handlers[ISR_COPRO] = idt_cso;
	idt_handlers[ISR_TSS] = idt_ts;
	idt_handlers[ISR_SEGNP] = idt_np;
	idt_handlers[ISR_STACK] = idt_ss;
	idt_handlers[ISR_GPFLT] = idt_gp;
	idt_handlers[ISR_PGFLT] = idt_pf;
	idt_handlers[ISR_FPERR] = idt_mf;
	idt_handlers[ISR_ALIGN] = idt_ac;
	idt_handlers[ISR_MCHK] = idt_mc;
	idt_handlers[ISR_SIMDERR] = idt_xm;

	pic_reset(PIC1_START_IRQ, PIC2_START_IRQ);
	memset(&idt, 0, sizeof(idt_entry) * IDT_ENTRIES);
	
	lidt(&IDT_DESC);
	return;
}

void idt_register(int intr, isr_t handler, int pl) {
	kassert(pl == PL_KERNEL || pl == PL_USER);

	interrupt_handlers[intr] = handler;

	if (pl == PL_KERNEL) {
		idt[intr] = make_idt_entry(idt_handlers[intr], SS_K_CODE, IDT_INT);
	} else if (pl == PL_USER) {
		idt[intr] = make_idt_entry(idt_handlers[intr], SS_U_CODE, IDT_INT);
	}
}
