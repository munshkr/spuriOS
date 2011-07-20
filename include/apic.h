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

/* Remember change some values in kernel/ap_trampoline.asm
 * when modifying this struct */
struct lapic_t {
	bool present;
	uint_32 id;
	void* base_addr;

	void (*enable)(struct lapic_t* self);
} __attribute__((__packed__));

typedef struct lapic_t lapic_t;

struct io_apic_t {
	bool present;
	uint_8 id;
	uint_8 version;
	void* addr;
} __attribute__((__packed__));

typedef struct io_apic_t io_apic_t;

void apic_send_startup_ipi(lapic_t* lapic, uint_8 dest_apic_id, uint_32 startup_offset);
void apic_clear_error_register(lapic_t* lapic);
void apic_send_init(lapic_t* lapic, uint_8 dest_apic_id, bool assert);
uint_32 apic_read32(void* lapic_base, uint_32 reg_offset);
void apic_write32(void* lapic_base, uint_32 reg_offset, uint_32 value);

bool apic_init(uint_32 bsp_id);
void apic_soft_enable(lapic_t* lapic);

#endif

#endif // __APIC_H__
