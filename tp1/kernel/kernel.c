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
	int a = 5;
	int b = 0;
	vga_printf("%d", a / b);
	return;
}
