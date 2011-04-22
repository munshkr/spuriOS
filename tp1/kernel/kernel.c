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

extern void* _end;

/* Entry-point del modo protegido luego de cargar los registros de
 * segmento y armar un stack */
void kernel_init(void* mmap_addr, size_t mmap_entries) {
	vga_init();
	gdt_init();
	
	idt_init();
	debug_init();

	breakpoint();

	// Raises #GP(40)
	__asm __volatile("jmp $40,$0");

	return;
}
