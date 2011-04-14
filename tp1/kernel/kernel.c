#include <pic.h>
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
void kernel_init(void) {
	cli();

	//gdt_init();	

	vga_putchar('a');
	//vga_clear();
	//vga_printf("Hello World!\n");
	//printf("Fruta \\c0A %d %s", 123, "hola");

	return;
}
