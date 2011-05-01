#include <pic.h>
#include <mmap.h>
#include <idt.h>
#include <vga.h>
#include <mm.h>
#include <sched.h>
#include <gdt.h>
#include <debug.h>
#include <loader.h>
#include <syscalls.h>
#include <i386.h>
#include <common.h>

extern void* _end;
extern pso_file task_task1_pso;

inline void enable_paging() {
	mm_page* kernel_page_dir = mm_dir_new();
	lcr3((uint_32) kernel_page_dir);	

	uint_32 cr0 = rcr0();
	cr0 |= CR0_PG;
	lcr0(cr0);
}

inline void go_idle() {
	debug_log("the kernel is going idle");

	/* This is all the information in the TSS, needed
	 * if an interrupt or exception occurs and a stack
	 * change needs to be performed */
	the_tss.ss0 = SS_K_DATA;
	the_tss.esp0 = esp(); 

	sti();	
	while (1) hlt();
}

/* Entry-point del modo protegido luego de cargar los registros de
 * segmento y armar un stack */
void kernel_init(mmap_entry_t* mmap_addr, size_t mmap_entries) {
	vga_init();
	gdt_init();
	idt_init();
	debug_init();

	mm_init(mmap_addr, mmap_entries);
	enable_paging();

	loader_init();
	sched_init();

	//loader_load(&task_task1_pso, PL_KERNEL);

	go_idle();
	return;
}
