#include <apic.h>
#include <processor.h>
#include <i386.h>
#include <debug.h>
#include <vga.h>
#include <mm.h>
#include <common.h>
#include <debug.h>
#include <tipos.h>

void apic_send_startup_ipi(lapic_t* lapic, uint_8 dest_apic_id,
	uint_32 startup_offset) {
	kassert((startup_offset & 0xFFF) == 0);
	void* apic_base = lapic->base_addr;
	apic_write32(apic_base, LAPIC_ICR_HIGH_OFFSET,
		((uint_32) dest_apic_id) << 24);
	apic_write32(apic_base, LAPIC_ICR_LOW_OFFSET,
		LAPIC_ICR_SIPI + (((startup_offset) >> 12) & 0xFF));
} 

void apic_clear_error_register(lapic_t* lapic) {
	apic_write32(lapic->base_addr, LAPIC_ESR_OFFSET, 0);
}

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

void apic_send_init(lapic_t* lapic, uint_8 dest_apic_id, bool assert) {
	void* apic_base = lapic->base_addr;
	apic_write32(apic_base, LAPIC_ICR_HIGH_OFFSET,
		((uint_32) dest_apic_id) << 24);

	uint_32 command = LAPIC_ICR_INIT + LAPIC_ICR_LVL_TRIG;
	if (assert)	command += LAPIC_ICR_ASSERT;
	apic_write32(apic_base, LAPIC_ICR_LOW_OFFSET, command);
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
