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
	gdt_init();

	vga_init();

	vga_printf("Hello World!\n");
	vga_printf("Fruta \\c0a%d %s\\c07, pero todo bien ahora\n", 123, "hola");

	int i;
	vga_attr.fld.light = TRUE;
	for (i = 0; i < 8; ++i) {
		vga_attr.fld.forecolor = i;
		if (i % 2 == 0) vga_attr.fld.backcolor = i;
		vga_printf("shake it\n");
	}

	// 80 is an invalid column, so it raises an assertion
	vga_set_pos(80, 12);
	vga_putchar('F');

	return;
}
