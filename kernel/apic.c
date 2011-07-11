#include <apic.h>
#include <processor.h>
#include <i386.h>
#include <debug.h>
#include <vga.h>
#include <mm.h>
#include <common.h>
#include <debug.h>
#include <tipos.h>

uint_32 apic_read32(void* lapic_base, uint_32 reg_offset) {
	kassert((reg_offset & 0xF) == 0);
	uint_32* reg = (uint_32*)(lapic_base + reg_offset);
	return *reg;
}

void apic_write32(void* lapic_base, uint_32 reg_offset, uint_32 value) {
	kassert((reg_offset & 0xF) == 0);
	uint_32* reg = (uint_32*)(lapic_base + reg_offset);
	*reg = value;
}

void apic_soft_enable(lapic_t* lapic) {
	uint_32 svr = apic_read32(lapic->base_addr, LAPIC_SVR_OFFSET);
	svr |= LAPIC_SVR_SOFT_ENABLE;
	apic_write32(lapic->base_addr, LAPIC_SVR_OFFSET, svr);
}

bool apic_init(uint_32 bsp_id) {
	cpuid_info_t cpuid_info;
	cpuid(0x1, 0x0, &cpuid_info);

	if (!(cpuid_info.edx & CPUID_01_FEATURE_APIC))
		return FALSE;

	debug_log("local apic ok according to cpuid");

	uint_64 apic_msr = read_msr(IA32_APIC_BASE_MSR);
	if (!(apic_msr & APIC_MSR_GLOBAL_ENABLE)) {
		return FALSE;
	}

	debug_log("local apic globally enabled");

	void* apic_base = processors[bsp_id].lapic.base_addr;
	mm_page* cr3 = (mm_page*) rcr3();

	debug_log("local apic physical address base is %p", apic_base);
	mm_map_frame(apic_base, apic_base, cr3, PL_KERNEL);
	mm_set_uncacheable((void*) apic_base, cr3);

	lapic_t* bsp_apic = &processors[bsp_id].lapic;
	bsp_apic->enable(bsp_apic);
	debug_log("local apic enabled by software");

	return TRUE;
}
