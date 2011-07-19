#include <processor.h>
#include <mpspec_def.h>
#include <tipos.h>
#include <vga.h>
#include <lib.h>
#include <common.h>
#include <debug.h>
#include <apic.h>
#include <mm.h>
#include <spinlock.h>
#include <gdt.h>

// Like Linux. See linux/arch/x86/kernel/mpparse.c
static struct mpf_intel mpf_found;
static struct mpc_table mpc_found;

uint_32 processor_bsp_id = 0;

processor_t processors[MAX_PROCESSORS] = {
		[0 ... MAX_PROCESSORS - 1] = {
			.present = FALSE,
			.is_the_bsp = FALSE,
			.stack_page = (void*) 0,
			.lapic = {
				.present = FALSE
			}
		}
	};

static uint_32 mpf_checksum(uint_8* mp, sint_32 len) {   
    uint_32 sum = 0;

    while (len--)
        sum += *mp++;

    return sum & 0xFF;
}

static int smp_check_mpc(struct mpc_table *mpc)
{
	if (strncmp(mpc->signature, MPC_SIGNATURE, 4)) {
		debug_log("SMP: bad signature [%c%c%c%c]!",
		       mpc->signature[0], mpc->signature[1],
		       mpc->signature[2], mpc->signature[3]);
		return 0;
	}

	if (mpf_checksum((unsigned char *)mpc, mpc->length)) {
		debug_log("SMP: checksum error for mpc table!");
		return 0;
	}

	if (mpc->spec != 0x01 && mpc->spec != 0x04) {
		debug_log("SMP: bad table version (%d)!!",
		       mpc->spec);
		return 0;
	}
	if (!mpc->lapic) {
		debug_log("SMP: null local APIC address!");
		return 0;
	}

	debug_log("SMP: apic physical base is 0x%x", mpc->lapic);

	return 1;
}

static void skip_entry(unsigned char **ptr, int *count, int size)
{
	*ptr += size;
	*count += size;
}

static void add_processor(uint_32 id, mpc_cpu* info) {
	if (info->cpuflag & CPU_BOOTPROCESSOR)
		processor_bsp_id = id;	

	processors[id] = (processor_t) {
		.present = TRUE,
		.is_the_bsp = (info->cpuflag & CPU_BOOTPROCESSOR ? TRUE : FALSE),

		.lapic = (lapic_t) {
			.present = TRUE,
			.id = info->apicid,
			.base_addr = (void*) mpc_found.lapic,
			.enable = apic_soft_enable
		},

		.the_tss = (tss) {
			// Never changes
			.ss0 = SS_K_DATA,
			.esp0 = K_STACK_TOP 
		}
	};
}

static int smp_read_mpc(struct mpc_table *mpc) {
	int processor_id = 0;
	int count = sizeof(*mpc);

	unsigned char *mpt = ((unsigned char *)mpc) + count;

	if (!smp_check_mpc(mpc))
		return 0;

	while (count < mpc->length) {
		switch (*mpt) {
		case MP_PROCESSOR: ;
			mpc_cpu* info = (mpc_cpu*) mpt;
			if (info->cpuflag & CPU_ENABLED) {
				add_processor(processor_id, info);
				processor_id++;
			}
			skip_entry(&mpt, &count, sizeof(mpc_cpu));
			break;
		case MP_BUS:
//			MP_bus_info((struct mpc_bus *)mpt);
			skip_entry(&mpt, &count, sizeof(struct mpc_bus));
			break;
		case MP_IOAPIC:
//			MP_ioapic_info((struct mpc_ioapic *)mpt);
			skip_entry(&mpt, &count, sizeof(struct mpc_ioapic));
			break;
		case MP_INTSRC:
//			vga_printf("\t(smp) int src found!\n");
//			mp_save_irq((struct mpc_intsrc *)mpt);
			skip_entry(&mpt, &count, sizeof(struct mpc_intsrc));
			break;
		case MP_LINTSRC:
//			MP_lintsrc_info((struct mpc_lintsrc *)mpt);
			skip_entry(&mpt, &count, sizeof(struct mpc_lintsrc));
			break;
		default:
			debug_log("SMP: invalid mpc table!");
			count = mpc->length;
			break;
		}
	}

	if (!processor_id)
		vga_printf("(smp) no processors registered!\n");
	return processor_id;
}

static bool smp_scan_config(uint_32 base,  uint_32 length) {
	uint_32* bp = (uint_32*) base;
    struct mpf_intel *mpf;

    while (length > 0) {
        mpf = (struct mpf_intel *)bp;
        if ((*bp == SMP_MAGIC_IDENT) && (mpf->length == 1) &&
            !mpf_checksum((unsigned char *)bp, 16) &&
			((mpf->specification == 1) || (mpf->specification == 4))) {

            mpf_found = *mpf;
            debug_log("SMP: found SMP MP-table at %p", mpf);
            return TRUE;
        }
        bp += 4;
        length -= 16;
    }
	return FALSE;
}

static inline uint_32 get_bios_ebda() {
    unsigned int address = *((unsigned short *) 0x40E);
    address <<= 4;
    return address; /* 0 means none */
}

static void show_processors() {
	uint_32 id = 0;
	for (id = 0; id < MAX_PROCESSORS; id++) {
		if (!processors[id].present)
			continue;
		debug_log("processor #%d: apic id = %d, %s",
			id, processors[id].lapic.id,
			(processors[id].is_the_bsp ? "bootstrap processor" : "application processor")
		);
	}
}

void processor_gather_mp_info() {
	if (!(smp_scan_config(0x0, 0x400) ||
        smp_scan_config(639 * 0x400, 0x400) ||
        smp_scan_config(0xE0000, 0x20000))) {

		uint_32 bios_ebda = get_bios_ebda();
		if (bios_ebda) {
			debug_log("SMP: extended bios data area at %x", bios_ebda);
			if (!smp_scan_config(bios_ebda, 0x400)) {
				return ;
			}
		} else {
			return ;
		}
	}

	debug_log("SMP: configuration table at %x", mpf_found.physptr);

	// We copy only the MPC header, not the entries
	mpc_found = *((struct mpc_table*)(mpf_found.physptr));

	smp_read_mpc((struct mpc_table*) mpf_found.physptr);
	show_processors();
}

extern void (ap_trampoline)();
void setup_warm_reset() {
	debug_log("SMP: setting up warm reset for APs");

	uint_8* shutdown_code = (uint_8*) 0xF; // 0x0:0xF
	*shutdown_code = 0xA;

	uint_32* trampoline_eip = (uint_32*) 0x467; // 0x40:0x67
	*trampoline_eip = (uint_32) ap_trampoline;	
}

void processor_smp_boot() {
	if (!apic_init(processor_bsp_id))
		return ;

	// Add BSP TSS's descriptor to the GDT
	processors[processor_bsp_id].tss_selector =
		gdt_add_tss(&processors[processor_bsp_id].the_tss);
	ltr(processors[processor_bsp_id].tss_selector);

	// Temporally map the page 0
	mm_map_frame((void*) 0, (void*) 0, (mm_page*) rcr3(), PL_KERNEL);

	// APIC enabled and ready
	setup_warm_reset();

	lapic_t* lapic = &processors[processor_bsp_id].lapic;

	uint_32 proc_id;
	for (proc_id = 0; proc_id < MAX_PROCESSORS; proc_id++) {
		if (processors[proc_id].present && !processors[proc_id].is_the_bsp) {
			processors[proc_id].tss_selector = gdt_add_tss(&processors[proc_id].the_tss);

			processors[proc_id].stack_page = mm_mem_kalloc();

			uint_8 dest_apic_id = processors[proc_id].lapic.id;

			// xAPIC Startup Sequence (what about 486DX2 APIC?)

			// ... INIT Assert/Deassert
			apic_send_init(lapic, dest_apic_id, TRUE);
			apic_send_init(lapic, dest_apic_id, FALSE);

			// ... SIPIs
			apic_clear_error_register(lapic);
			apic_send_startup_ipi(lapic, dest_apic_id, (uint_32) ap_trampoline);
		}
	}

	mm_unmap_page((void*) 0, (mm_page*) rcr3());
}

void processor_ap_kinit(processor_t* self) {
	ltr(self->tss_selector);
	debug_log("SMP: processor [lapic #%d] bootup finished", self->lapic.id);

	while(1); 
}
