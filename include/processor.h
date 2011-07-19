#ifndef __PROCESSOR_H__
#define __PROCESSOR_H__

#ifdef __KERNEL__

#include <tipos.h>
#include <apic.h>
#include <tss.h>

/* Remember change some values in kernel/ap_trampoline.asm
 * when modifying this struct */
struct processor_t {
	bool		present;
	bool		is_the_bsp;
	void*		stack_page;
	lapic_t		lapic;
	uint_16		tss_selector;
	tss			the_tss;
} __attribute__((__packed__));

typedef struct processor_t processor_t;

#define BOOTSTRAP_PROCESSOR 0

#define MAX_PROCESSORS 4
extern processor_t processors[];
extern uint_32 processor_bsp_id;

void processor_smp_boot();
void processor_gather_mp_info();

#endif

#endif // __PROCESSOR_H__
