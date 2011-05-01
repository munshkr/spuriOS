#include <tipos.h>
#include <i386.h>
#include <gdt.h>
#include <isr.h>
#include <idt.h>
#include <pic.h>
#include <common.h>
#include <debug.h>

// Defined in isr.c
extern isr_t interrupt_handlers[];

// Defined in interrup.asm
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

extern void (*irq0)(void);
extern void (*irq1)(void);
extern void (*irq2)(void);
extern void (*irq3)(void);
extern void (*irq4)(void);
extern void (*irq5)(void);
extern void (*irq6)(void);
extern void (*irq7)(void);
extern void (*irq8)(void);
extern void (*irq9)(void);
extern void (*irq10)(void);
extern void (*irq11)(void);
extern void (*irq12)(void);
extern void (*irq13)(void);
extern void (*irq14)(void);
extern void (*irq15)(void);

static const uint_32 IDT_ATTR_DPL_[4] = { IDT_ATTR_DPL0, IDT_ATTR_DPL1, IDT_ATTR_DPL2, IDT_ATTR_DPL3 };

// Macro para crear una entrada de la IDT dando offset(32), selector(16) y attr(16).
#define make_idt_entry(offset, select, attr) \
	(idt_entry){{((uint_32)(offset) & 0xFFFF) | ((uint_32)(select) << 16), \
	((uint_32)(attr) & 0xFFFF) | ((uint_32)(offset) & 0xFFFF0000) }}

#define idt_entry_null make_idt_entry(0,0,0)

#define IDT_INT IDT_ATTR_P | IDT_ATTR_S_ON | IDT_ATTR_D_32 | IDT_ATTR_TYPE_INT
#define IDT_EXP IDT_ATTR_P | IDT_ATTR_S_ON | IDT_ATTR_D_32 | IDT_ATTR_TYPE_EXP

#define IDT_ENTRIES 256

#define PIC1_START_IRQ 0x20
#define PIC2_START_IRQ 0x28

idt_entry idt[IDT_ENTRIES] = {};

idt_descriptor IDT_DESC = {sizeof(idt)-1, (uint_32)&idt};

// Array of pointers to the ASM-handlers
void (*idt_handlers[IDT_ENTRIES])(void) = {};


void idt_init(void) {
	debug_log("initializing interrupt handling");

	// Relate ASM-handlers to their corresponding interrupt number
	idt_handlers[ISR_DIVIDE]  = (void *) &idt_de;
	idt_handlers[ISR_DEBUG]   = (void *) &idt_db;
	idt_handlers[ISR_NMI]     = (void *) &idt_nmi;
	idt_handlers[ISR_BRKPT]   = (void *) &idt_bp;
	idt_handlers[ISR_OFLOW]   = (void *) &idt_of;
	idt_handlers[ISR_BOUND]   = (void *) &idt_br;
	idt_handlers[ISR_ILLOP]   = (void *) &idt_ud;
	idt_handlers[ISR_DEVICE]  = (void *) &idt_nm;
	idt_handlers[ISR_DBLFLT]  = (void *) &idt_df;
	idt_handlers[ISR_COPRO]   = (void *) &idt_cso;
	idt_handlers[ISR_TSS]     = (void *) &idt_ts;
	idt_handlers[ISR_SEGNP]   = (void *) &idt_np;
	idt_handlers[ISR_STACK]   = (void *) &idt_ss;
	idt_handlers[ISR_GPFLT]   = (void *) &idt_gp;
	idt_handlers[ISR_PGFLT]   = (void *) &idt_pf;
	idt_handlers[ISR_FPERR]   = (void *) &idt_mf;
	idt_handlers[ISR_ALIGN]   = (void *) &idt_ac;
	idt_handlers[ISR_MCHK]    = (void *) &idt_mc;
	idt_handlers[ISR_SIMDERR] = (void *) &idt_xm;

	idt_handlers[ISR_IRQ0]  = (void *) &irq0;
	idt_handlers[ISR_IRQ1]  = (void *) &irq1;
	idt_handlers[ISR_IRQ2]  = (void *) &irq2;
	idt_handlers[ISR_IRQ3]  = (void *) &irq3;
	idt_handlers[ISR_IRQ4]  = (void *) &irq4;
	idt_handlers[ISR_IRQ5]  = (void *) &irq5;
	idt_handlers[ISR_IRQ6]  = (void *) &irq6;
	idt_handlers[ISR_IRQ7]  = (void *) &irq7;
	idt_handlers[ISR_IRQ8]  = (void *) &irq8;
	idt_handlers[ISR_IRQ9]  = (void *) &irq9;
	idt_handlers[ISR_IRQ10]  = (void *) &irq10;
	idt_handlers[ISR_IRQ11]  = (void *) &irq11;
	idt_handlers[ISR_IRQ12]  = (void *) &irq12;
	idt_handlers[ISR_IRQ13]  = (void *) &irq13;
	idt_handlers[ISR_IRQ14]  = (void *) &irq14;
	idt_handlers[ISR_IRQ15]  = (void *) &irq15;

	// Initialize and remap IRQs in both PICs
	pic_reset(PIC1_START_IRQ, PIC2_START_IRQ);

	// Initialize IDT with empty descriptors
	memset(&idt, 0, sizeof(idt_entry) * IDT_ENTRIES);

	// Load IDT base pointer
	lidt(&IDT_DESC);

	return;
}

void idt_register(int intr, isr_t handler, int pl) {
	// Privilege level must be KERNEL or USER
	kassert(pl == PL_KERNEL || pl == PL_USER);

	// Interrupt number must range from 0 to 255
	kassert(intr >= 0 && intr < 256);

	// ASM-handler wrapper must be defined in `idt_handlers` array
	// FIXME INT 15 is reserved but is not used and thus not defined in
	// interrup.asm...
	//kassert(idt_handlers[intr] != NULL);

	// Register C-handler
	interrupt_handlers[intr] = handler;

	// Register ASM-handler wrapper in IDT
	if (pl == PL_KERNEL) {
		idt[intr] = make_idt_entry((void *) idt_handlers[intr], SS_K_CODE, IDT_INT);
	} else if (pl == PL_USER) {
		idt[intr] = make_idt_entry((void *) idt_handlers[intr], SS_U_CODE, IDT_INT);
	}
}
