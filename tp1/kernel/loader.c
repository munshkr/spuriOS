#include <loader.h>
#include <timer.h>
#include <idt.h>
#include <common.h>
#include <i386.h>
#include <vga.h>

#define PSO_SIGNATURE_0 0x50
#define PSO_SIGNATURE_1 0x53
#define PSO_SIGNATURE_2 0x4F
#define PSO_SIGNATURE_3 0x00

static void timer_handler(registers_t regs);

task task_table[MAX_PID];

/* pid "actual" */
uint_32 cur_pid = 0;

void loader_init(void) {
	memset(task_table, 0, sizeof(task) * MAX_PID);

	idt_register(ISR_IRQ0, timer_handler, PL_KERNEL);
	timer_init(1500);
}

pid loader_load(pso_file* f, int pl) {
	if (f->signature[0] != PSO_SIGNATURE_0 || f->signature[1] != PSO_SIGNATURE_1 || f->signature[2] != PSO_SIGNATURE_2 || f->signature[3] != PSO_SIGNATURE_3) {
		vga_printf("\\c0CWrong file signature.\n");
	} else {
		vga_printf("Task:\n\tmem_start: %x\n\tmem_end_disk: %x\n\tmem_end: %x\n\t_main: %x\n\tdata: %x\n", (unsigned int)f->mem_start, (unsigned int)f->mem_end_disk, (unsigned int)f->mem_end, (unsigned int)f->_main, (unsigned int)f->data);
	}
	return 0;

}

void loader_enqueue(int* cola) {
}

void loader_unqueue(int* cola) {
}

void loader_exit(void) {
}

void loader_switchto(pid pd) {
}


static void timer_handler(registers_t regs) {
	tick++;
	timer_draw_clock();
}
