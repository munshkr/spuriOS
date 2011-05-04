#include <loader.h>
#include <timer.h>
#include <idt.h>
#include <common.h>
#include <i386.h>
#include <debug.h>
#include <gdt.h>
#include <sched.h>
#include <vga.h>
#include <mm.h>

const char PSO_SIGNATURE[4] = "PSO";

extern void (timer_handler)();

#define FREE_PCB_PID 0xFFFFFFFF
#define USER_MEMORY_START 0x400000

pcb_t processes[MAX_PID];
pid cur_pid;
pid tmp_pid;

inline void initialize_process_list() {
	uint_32 id;
	for (id = 0; id < MAX_PID; id++) {
		processes[id].id = FREE_PCB_PID;
	}
}

void loader_init(void) {
	debug_log("initializing loader");

	idt_register_asm(ISR_IRQ0, timer_handler, PL_KERNEL);
	timer_init(1500);

	initialize_process_list();

	cur_pid = IDLE_PID;
	processes[IDLE_PID].id = IDLE_PID;
	processes[IDLE_PID].privilege_level = PL_KERNEL;
	processes[IDLE_PID].cr3 = rcr3();
}

static inline void print_pso_file(pso_file* f) {
	vga_printf("Task:\n");
	vga_printf("\tmem_start: %x\n", f->mem_start);
	vga_printf("\tmem_end_disk: %x\n", f->mem_end_disk);
	vga_printf("\tmem_end: %x\n", f->mem_end);
	vga_printf("\t_main: %x\n", (unsigned int) f->_main);
	vga_printf("\tdata: %x\n", (unsigned int) f->data);
}

static inline int get_new_pid() {
	int pid;
	for (pid = 0; pid < MAX_PID; pid++) {
		if (processes[pid].id == FREE_PCB_PID) break;
	}
	// No more free PIDs! (consider reducing load or enlarging MAX_PID...)
	kassert(pid < MAX_PID);
	return pid;
}

pid loader_load(pso_file* f, int pl) {
	// Verify file signature
	kassert(f->signature[0] == PSO_SIGNATURE[0] &&
			f->signature[1] == PSO_SIGNATURE[1] &&
			f->signature[2] == PSO_SIGNATURE[2] &&
			f->signature[3] == PSO_SIGNATURE[3]);

	print_pso_file(f);

	int pid = get_new_pid();

	processes[pid].id = pid;
	processes[pid].privilege_level = pl;

	// Temporal page to map new frames and initialize them
	mm_page* current_pdt = (mm_page*) processes[cur_pid].cr3;

	void* temp_page;
	void* old_frame = NULL;
	if (cur_pid == IDLE_PID) {
		// Use first page for users (it's empty always)
		temp_page = (void*) USER_MEMORY_START;
	} else {
		// Stash the frame used in the first page of
		// the currently running task, because we'll use it
		// for initializing the new task's frames.

		// FIXME We know f->mem_start == USER_MEMORY_START for now,
		// so no page table may be created when we map that page.
		// We should store f->mem_start in the PCB to know where
		// the first page is, to avoid a possible page table alloc.
		temp_page = (void*) USER_MEMORY_START;
		old_frame = get_frame_from_page(temp_page, current_pdt);
	}

	// Create new Page Directory Table
	mm_page* new_pdt = mm_dir_new();
	processes[pid].cr3 = (uint_32) new_pdt;

	// Kernel stack frame
	void* k_stack_frame = mm_mem_alloc();
	map_frame(k_stack_frame, (void*) TASK_K_STACK_ADDRESS, new_pdt, PL_KERNEL);

	map_frame(k_stack_frame, temp_page, current_pdt, PL_KERNEL);
	memset(temp_page, 0, PAGE_SIZE);

	if (pl == PL_KERNEL) {
		((uint_32*) temp_page)[1023] = TASK_DEFAULT_EFLAGS; // EFLAGS
		((uint_32*) temp_page)[1022] = SS_K_CODE; // CS
		((uint_32*) temp_page)[1021] = (uint_32) f->_main; // EIP

		((uint_32*) temp_page)[1016] = (uint_32)(TASK_K_STACK_ADDRESS + 1020 * 4); // ESP
		((uint_32*) temp_page)[1015] = (uint_32)(TASK_K_STACK_ADDRESS + 1023 * 4); // EBP

		processes[pid].esp = (uint_32)(TASK_K_STACK_ADDRESS + 1013 * 4); // "In switch" ESP
	} else {
		// User stack frame
		void* u_stack_frame = mm_mem_alloc();
		map_frame(u_stack_frame, (void*) TASK_U_STACK_ADDRESS, new_pdt, PL_USER);

		((uint_32*) temp_page)[1023] = SS_U_DATA | PL_USER; // SS (USER)
		((uint_32*) temp_page)[1022] = (uint_32)(TASK_U_STACK_ADDRESS + 1024 * 4); // USER ESP

		((uint_32*) temp_page)[1021] = TASK_DEFAULT_EFLAGS; // EFLAGS
		((uint_32*) temp_page)[1020] = SS_U_CODE | PL_USER; // CS (USER)
		((uint_32*) temp_page)[1019] = (uint_32) f->_main; // EIP

		((uint_32*) temp_page)[1014] = (uint_32)(TASK_K_STACK_ADDRESS + 1018 * 4); // ESP
		((uint_32*) temp_page)[1013] = (uint_32)(TASK_U_STACK_ADDRESS + 1024 * 4); // EBP (USER)

		processes[pid].esp = (uint_32)(TASK_K_STACK_ADDRESS + 1011 * 4); // "In switch" ESP
	}

	// Calculate number of pages required for the new task
	uint_32 total_pages = (f->mem_end - f->mem_start) / PAGE_SIZE;
	if ((f->mem_end - f->mem_start) % PAGE_SIZE) total_pages++;

	void* task_mem_p = (sint_8*) f->mem_start;
	void* task_data_p = (sint_8*) f;

	int i;
	for (i = 0; i < total_pages; ++i) {
		void* frame = mm_mem_alloc();
		map_frame(frame, task_mem_p, new_pdt, pl);

		map_frame(frame, temp_page, current_pdt, PL_KERNEL);

		// Copy task code from memory or initialize with zeros the new frames
		if (task_mem_p >= (void*) f->mem_end_disk) {
			memset(temp_page, 0, PAGE_SIZE);
		} else {
			size_t bytes = MIN(PAGE_SIZE, (void*) f->mem_end_disk - task_mem_p);
			memcpy(task_data_p, temp_page, bytes);
			memset(temp_page + bytes, 0, PAGE_SIZE - bytes);
		}

		task_mem_p  += PAGE_SIZE;
		task_data_p += PAGE_SIZE;
	}

	unmap_page(temp_page, current_pdt);

	// Restore original frame used by current task
	if (cur_pid != IDLE_PID) {
		map_frame(old_frame, temp_page, current_pdt, processes[cur_pid].privilege_level);
	}

	sched_load(pid);

	return pid;
}

void loader_enqueue(int* cola) {
}

void loader_unqueue(int* cola) {
}

void loader_exit(void) {
}

