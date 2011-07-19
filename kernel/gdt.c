#include <tipos.h>
#include <gdt.h>
#include <i386.h>
#include <debug.h>
#include <tss.h>

gdt_entry gdt[GDT_COUNT] = {
	make_gdt_entry(0, 0, 0),
	make_gdt_entry(0, 0xFFFFF, GDT_ATTR_SEG | GDT_ATTR_SEG_CODE | GDT_ATTR_DPL0), // SEG_CODE_0
	make_gdt_entry(0, 0xFFFFF, GDT_ATTR_SEG | GDT_ATTR_SEG_DATA | GDT_ATTR_DPL0), // SEG_DATA_0
	make_gdt_entry(0, 0xFFFFF, GDT_ATTR_SEG | GDT_ATTR_SEG_CODE | GDT_ATTR_DPL3), // SEG_CODE_3
	make_gdt_entry(0, 0xFFFFF, GDT_ATTR_SEG | GDT_ATTR_SEG_DATA | GDT_ATTR_DPL3)  // SEG_DATA_3
};

// NOTE Replaced by the GDT_DESC defined in kernel/kinit.asm
//gdt_descriptor GDT_DESC = {sizeof(gdt)-1, (uint_32)&gdt};

uint_16 gdt_next_free_entry = 5;

uint_16 gdt_add_tss(tss* the_tss) {
	gdt[gdt_next_free_entry] = make_gdt_entry(the_tss, sizeof(*the_tss) - 1,
		GDT_ATTR_P | GDT_ATTR_DPL3 | GDT_ATTR_TYPE_TSS);

	uint_16 selector = gdt_next_free_entry << 3;
	gdt_next_free_entry++;
	return selector;
}

