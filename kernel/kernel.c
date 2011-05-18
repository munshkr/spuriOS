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
#include <common.h>
#include <kbd.h>
#include <device.h>

#define __UNIT_TESTING__

const char* fancy_logo[5] = {
	"    //   ) )                                //   ) ) //   ) )",
	"   ((         ___               __     ( ) //   / / ((       ",
	"     \\\\     //   ) ) //   / / //  ) ) / / //   / /    \\\\ ",
	"       ) ) //___/ / //   / / //      / / //   / /       ) )  ",
	"((___ / / //       ((___( ( //      / / ((___/ / ((___ / /   ",
};

extern void* _end;
extern pso_file task_ut_cp2user_pso;
extern pso_file task_ut_getpid_pso;
extern pso_file task_task_kbd_pso;
extern pso_file task_ut_dummy_pso;
extern pso_file task_task_sin_pso;
extern pso_file task_task_pf_pso;
extern pso_file task_task_funky_pso;
extern pso_file task_ut_cpuid_pso;
extern pso_file task_ut_palloc_pso;
extern pso_file task_ut_con_pso;
extern pso_file task_serial_pso;

extern pso_file task_console_pso;

inline void enable_paging() {
	mm_page* kernel_page_dir = mm_dir_new();
	lcr3((uint_32) kernel_page_dir);

	uint_32 cr0 = rcr0();
	cr0 |= CR0_PG;
	lcr0(cr0);
}

inline void be_task() {
	ltr(SS_TSS);
}

inline void go_idle() {
	debug_log("the kernel is going idle");

	// Never changes
	the_tss.ss0 = SS_K_DATA;

	processes[IDLE_PID].esp = esp();
	be_task();

	sti();
	while (1) hlt();
}

void print_logo(void) {
	int i;
	for (i = 0; i < 5; i++) {
		vga_attr.fld.forecolor = i + 2;
		vga_attr.fld.light = TRUE;
		vga_printf("%s\n", fancy_logo[i]);
	}
	vga_printf("\n");
	vga_reset_colors();
}

/* Entry-point del modo protegido luego de cargar los registros de
 * segmento y armar un stack */
void kernel_init(mmap_entry_t* mmap_addr, size_t mmap_entries) {
	vga_init();
	gdt_init();
	idt_init();
	debug_init();

	mm_init(mmap_addr, mmap_entries);
	enable_paging();

	loader_init();
	sched_init();

	kbd_init();
	syscalls_init();

	device_init();

	print_logo();

	#ifdef __UNIT_TESTING__
	debug_log("running unitary tests");
	loader_load(&task_ut_dummy_pso, PL_KERNEL);
	loader_load(&task_ut_getpid_pso, PL_USER);
	loader_load(&task_ut_palloc_pso, PL_USER);
	loader_load(&task_ut_cpuid_pso, PL_USER);
	loader_load(&task_ut_cp2user_pso, PL_USER);
	loader_load(&task_ut_con_pso, PL_USER);
	#endif

	loader_load(&task_console_pso, PL_USER);
	loader_load(&task_serial_pso, PL_USER);

	go_idle();
	return;
}
