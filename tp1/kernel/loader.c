#include <loader.h>
#include <timer.h>
#include <idt.h>
#include <common.h>
#include <i386.h>

static void timer_handler(registers_t regs);

task task_table[MAX_PID];

/* pid "actual" */
uint_32 cur_pid = 0;

void loader_init(void) {
	idt_register(ISR_IRQ0, timer_handler, PL_KERNEL);
	timer_init(1500);
}

pid loader_load(pso_file* f, int pl) {
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
