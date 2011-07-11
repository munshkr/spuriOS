#ifndef __APIC_H__
#define __APIC_H__

#ifdef __KERNEL__

// APIC MSR
#define IA32_APIC_BASE_MSR		0x1B
#define APIC_MSR_GLOBAL_ENABLE	2048

// Registers. See Intel 3A table 8-1
#define LAPIC_SVR_OFFSET		0xF0
#define LAPIC_ICR_LOW_OFFSET	0x300
#define LAPIC_ICR_HIGH_OFFSET	0x310
#define LAPIC_ESR_OFFSET		0x370

// ICR (Interrupt Control Register)
#define LAPIC_ICR_INIT			0x500
#define LAPIC_ICR_SIPI			0x600
#define LAPIC_ICR_ASSERT		0x4000
#define LAPIC_ICR_LVL_TRIG		0x8000

// SVR (Spurios Vector Register)
#define LAPIC_SVR_SOFT_ENABLE	0x100

#include <tipos.h>

struct lapic_t {
	bool present;
	uint_32 id;
	void* base_addr;

	void (*enable)(struct lapic_t* self);
};

typedef struct lapic_t lapic_t;

uint_32 apic_read32(void* lapic_base, uint_32 reg_offset);
void apic_write32(void* lapic_base, uint_32 reg_offset, uint_32 value);

bool apic_init(uint_32 bsp_id);
void apic_soft_enable(lapic_t* lapic);

#endif

#endif // __APIC_H__
