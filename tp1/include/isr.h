#ifndef __ISR_H__
#define __ISR_H__


#include <gdt.h>
#include <idt.h>

typedef struct str_exp_state {
	uint_32 gs;
	uint_32 fs;
	uint_32 es;
	uint_32 ds;
	uint_32 ss;
	uint_32 edi;
	uint_32 esi;
	uint_32 ebp;
	uint_32 esp;
	uint_32 ebx;
	uint_32 edx;
	uint_32 ecx;
	uint_32 eax;
	uint_32 errcode;
	uint_32 org_eip;
	uint_32 org_cs;
	uint_32 eflags;
	uint_32 org_esp;
	uint_32 org_ss;
} __attribute__((__packed__)) exp_state;

extern void (isr_de)(void);
extern void (isr_db)(void);
extern void (isr_nmi)(void);
extern void (isr_bp)(void);
extern void (isr_of)(void);
extern void (isr_br)(void);
extern void (isr_ud)(void);
extern void (isr_nm)(void);
extern void (isr_df)(void);
extern void (isr_cso)(void);
extern void (isr_ts)(void);
extern void (isr_np)(void);
extern void (isr_ss)(void);
extern void (isr_gp)(void);
extern void (isr_pf)(void);
extern void (isr_mf)(void);
extern void (isr_ac)(void);
extern void (isr_mc)(void);
extern void (isr_xm)(void);
#endif
