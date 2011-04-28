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
void kernel_init(mmap_entry_t* mmap_addr, size_t mmap_entries) {
	vga_init();
	gdt_init();
	idt_init();
	debug_init();

	mmap_entry_t* entry = (mmap_entry_t *) mmap_addr;
	int i;
	for (i = 0; i < mmap_entries; ++i, ++entry) {
		vga_printf("Entry %u\n", i);
		vga_printf("\t.addr: %x\n", (uint_32) entry->addr);
		vga_printf("\t.len: %u\n", (uint_32) entry->len);
		vga_printf("\t.type: %d", entry->type);
		if (entry->type == MMAP_MEMORY_AVAILABLE) {
			vga_printf("available\n");
		} else {
			vga_printf("reserved\n");
		}
	}

	breakpoint();

	mm_init(mmap_addr, mmap_entries);

	void* page = mm_mem_kalloc();
	vga_printf("Pagina recibida: \\c0F%x\n", (unsigned int)page);
	mm_mem_kalloc();
	mm_mem_free(page);
	page = mm_mem_kalloc();
	vga_printf("Pagina recibida: \\c0F%x\n", (unsigned int)page);
	page = mm_mem_alloc();
	vga_printf("Pagina recibida: \\c0F%x\n", (unsigned int)page);

	vga_clear();
	mm_print_map();

	return;
}
