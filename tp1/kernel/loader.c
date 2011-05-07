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
extern void (task_init)();
extern void (task_switch)();

pcb_t processes[MAX_PID];
pid cur_pid;
pid tmp_pid;

slept_task sleeping[MAX_PID];
pid first_slept;

inline void initialize_process_list() {
	uint_32 id;
	for (id = 0; id < MAX_PID; id++) {
		processes[id].id = FREE_PCB_PID;
	}
}

inline void initialize_sleeping_list() {
	uint_32 id;
	for (id = 0; id < MAX_PID; id++) {
		sleeping[id].id = FREE_PCB_PID;
		sleeping[id].time = 0;
		sleeping[id].next = FREE_PCB_PID;
	}
	first_slept = FREE_PCB_PID;
}

static void page_fault_handler(registers_t* regs) {
	if (processes[cur_pid].privilege_level == PL_USER) {
		vga_printf("Page fault! Killed %u\n", cur_pid);
		loader_exit();
	} else {
		debug_kernelpanic(regs);
	}
}

void loader_init(void) {
	debug_log("initializing loader");

	idt_register_asm(ISR_IRQ0, timer_handler, PL_KERNEL);
	timer_init(1000);

	initialize_process_list();

	initialize_sleeping_list();

	cur_pid = IDLE_PID;
	processes[IDLE_PID].id = IDLE_PID;
	processes[IDLE_PID].privilege_level = PL_KERNEL;
	processes[IDLE_PID].cr3 = rcr3();

	idt_register(ISR_PGFLT, page_fault_handler, PL_KERNEL);
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

inline uint_32 data_pages_needed_for(pso_file* f) {
	uint_32 pages = (f->mem_end - f->mem_start) / PAGE_SIZE;
	if ((f->mem_end - f->mem_start) % PAGE_SIZE) pages++;
	return pages;
}

inline void create_stacks_for(pso_file* f, mm_page* current_pdt, mm_page* new_pdt,
	uint_32 pl, void* temp_page, pcb_t* new_proc) {

	void* kernel_stack_frame = mm_mem_alloc();
	mm_map_frame(kernel_stack_frame, (void*) TASK_K_STACK_ADDRESS, new_pdt, PL_KERNEL);

	mm_map_frame(kernel_stack_frame, temp_page, current_pdt, PL_KERNEL);
	memset(temp_page, 0, PAGE_SIZE);

	if (pl == PL_KERNEL) {
		((uint_32*) temp_page)[1023] = TASK_DEFAULT_EFLAGS; // EFLAGS
		((uint_32*) temp_page)[1022] = SS_K_CODE; // CS
		((uint_32*) temp_page)[1021] = (uint_32) f->_main; // EIP
		((uint_32*) temp_page)[1020] = (uint_32) task_init; // task_swith ret EIP

		((uint_32*) temp_page)[1015] = (uint_32)(TASK_K_STACK_ADDRESS + 1019 * 4); // ESP
		((uint_32*) temp_page)[1014] = (uint_32)(TASK_K_STACK_ADDRESS + 1022 * 4); // EBP

		new_proc->esp = (uint_32)(TASK_K_STACK_ADDRESS + 1012 * 4); // "In switch" ESP
	} else {
		void* user_stack_frame = mm_mem_alloc();
		mm_map_frame(user_stack_frame, (void*) TASK_U_STACK_ADDRESS, new_pdt, PL_USER);

		((uint_32*) temp_page)[1023] = SS_U_DATA | PL_USER; // SS (USER)
		((uint_32*) temp_page)[1022] = (uint_32)(TASK_U_STACK_ADDRESS + 1024 * 4); // USER ESP

		((uint_32*) temp_page)[1021] = TASK_DEFAULT_EFLAGS; // EFLAGS
		((uint_32*) temp_page)[1020] = SS_U_CODE | PL_USER; // CS (USER)
		((uint_32*) temp_page)[1019] = (uint_32) f->_main; // EIP
		((uint_32*) temp_page)[1018] = (uint_32) task_init; // task_swith ret EIP

		((uint_32*) temp_page)[1013] = (uint_32)(TASK_K_STACK_ADDRESS + 1017 * 4); // ESP
		((uint_32*) temp_page)[1012] = (uint_32)(TASK_U_STACK_ADDRESS + 1024 * 4); // EBP (USER)

		new_proc->esp = (uint_32)(TASK_K_STACK_ADDRESS + 1010 * 4); // "In switch" ESP
	}

}

inline uint_32 copy_data_from(pso_file* f, mm_page* current_pdt, mm_page* new_pdt,
	void* temp_page, uint_32 pl) {
	uint_32 total_pages = data_pages_needed_for(f);

	void* task_mem_p = (sint_8*) f->mem_start;
	void* task_data_p = (sint_8*) f;

	int i;
	for (i = 0; i < total_pages; ++i) {
		void* frame = mm_mem_alloc();
		mm_map_frame(frame, task_mem_p, new_pdt, pl);

		mm_map_frame(frame, temp_page, current_pdt, PL_KERNEL);

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

	return total_pages;
}

inline void create_temp_mapping_on(void** temp_page, void** old_frame,
	mm_page* current_pdt) {

	if (cur_pid == IDLE_PID) {
		// Use first page for users (it's empty always)
		*(uint_32**)temp_page = (uint_32*) USER_MEMORY_START;
	} else {
		// FIXME We know f->mem_start == USER_MEMORY_START for now,
		// so no page table may be created when we map that page.
		// We should store f->mem_start in the PCB to know where
		// the first page is, to avoid a possible page table alloc.
		*(uint_32**)temp_page = (uint_32*) USER_MEMORY_START;
		*(uint_32**)old_frame = mm_frame_from_page(temp_page, current_pdt);
	}
}

inline void undo_temp_mapping_on(void* temp_page, void* old_frame,
	mm_page* current_pdt) {
	mm_unmap_page(temp_page, current_pdt);
	if (cur_pid != IDLE_PID) {
		mm_map_frame(old_frame, temp_page, current_pdt, processes[cur_pid].privilege_level);
	}
}

inline void check_signature_of(pso_file* f) {
	kassert(f->signature[0] == PSO_SIGNATURE[0] &&
			f->signature[1] == PSO_SIGNATURE[1] &&
			f->signature[2] == PSO_SIGNATURE[2] &&
			f->signature[3] == PSO_SIGNATURE[3]);
}

pid loader_load(pso_file* f, uint_32 pl) {
	check_signature_of(f);
	uint_32 pid = get_new_pid();

	processes[pid].id = pid;
	processes[pid].privilege_level = pl;
	processes[pid].prev = processes[pid].next = FREE_PCB_PID;

	mm_page* current_pdt = (mm_page*) processes[cur_pid].cr3;

	void* temp_page;
	void* old_frame = NULL;

	create_temp_mapping_on(&temp_page, &old_frame, current_pdt);

	mm_page* new_pdt = mm_dir_new();
	processes[pid].cr3 = (uint_32) new_pdt;

	create_stacks_for(f, current_pdt, new_pdt, pl, temp_page, &processes[pid]);
	uint_32 total_pages = copy_data_from(f, current_pdt, new_pdt, temp_page, pl);

	// Store address for fast palloc
	processes[pid].next_empty_page_addr = USER_MEMORY_START + (total_pages * PAGE_SIZE);

	undo_temp_mapping_on(temp_page, old_frame, current_pdt);

	sched_load(pid);

	return pid;
}

void loader_enqueue(pid* cola) {
	pid local_tmp_pid;
	if (*cola == FREE_QUEUE) {
		*cola = cur_pid;
	} else {
		local_tmp_pid = *cola;
		while(processes[local_tmp_pid].next != FREE_PCB_PID) {
			local_tmp_pid = processes[local_tmp_pid].next;
		}
		processes[local_tmp_pid].next = cur_pid;
		processes[cur_pid].prev = local_tmp_pid;
	}
	tmp_pid = sched_block();
	task_switch();
}

void loader_unqueue(pid* cola) {
	pid local_tmp_pid;
	if (*cola != FREE_QUEUE) {
		if (processes[*cola].next != FREE_PCB_PID) {
			local_tmp_pid = processes[*cola].next;
			processes[local_tmp_pid].prev = FREE_PCB_PID;
			processes[*cola].next = FREE_PCB_PID;
			sched_unblock(*cola);
			*cola = local_tmp_pid;
		} else {
			sched_unblock(*cola);
			*cola = FREE_QUEUE;
		}
	}
}

void loader_sleep(uint_32 time) {
	pid tmp_slept;
	pid prev_slept = FREE_PCB_PID;
	if (time <= 0) {
		return;
	}

	//loader_print_raw_sleeping();
	//loader_print_sleeping();
	//breakpoint();

	if (first_slept == FREE_PCB_PID) {
//		breakpoint();
		first_slept = cur_pid;
		sleeping[cur_pid].id = cur_pid;
		sleeping[cur_pid].time = time;
		sleeping[cur_pid].next = FREE_PCB_PID;
	} else {
//		breakpoint();
		tmp_slept = first_slept;
		while (tmp_slept != FREE_PCB_PID) {
			if (sleeping[tmp_slept].time < time) {
				time -= sleeping[tmp_slept].time;
			} else {
				break;
			}
//			breakpoint();
			prev_slept = tmp_slept;
			tmp_slept = sleeping[tmp_slept].next;
		}
		if (tmp_slept == FREE_PCB_PID) {
//			breakpoint();
			sleeping[prev_slept].next = cur_pid;
			sleeping[cur_pid].id = cur_pid;
			sleeping[cur_pid].time = time;
			sleeping[cur_pid].next = FREE_PCB_PID;
		} else {
//			breakpoint();
			if (prev_slept == FREE_PCB_PID) {
//				breakpoint();
				first_slept = cur_pid;
			} else {
//				breakpoint();
				sleeping[prev_slept].next = cur_pid;
			}
//			breakpoint();
			sleeping[cur_pid].id = cur_pid;
			sleeping[cur_pid].time = time;
			sleeping[cur_pid].next = tmp_slept;
			sleeping[tmp_slept].time -= time;
		}
	}

	tmp_pid = sched_block();
	task_switch();
}

void loader_print_sleeping() {
	pid tmp_slept = first_slept;
	vga_printf("DeltaQueue->");
	while (tmp_slept != FREE_PCB_PID) {
		vga_printf("{%d, %d}->", tmp_slept, sleeping[tmp_slept].time);
		tmp_slept = sleeping[tmp_slept].next;
	}
	vga_printf("X\n");
}

void loader_print_raw_sleeping() {
	vga_printf("first_slept = %d\n", first_slept);
	int i;
	for (i = 0; i < 5; i++) {
		vga_printf("%d:\n\tid: %d\n\ttime: %d\n\tnext: %d\n", i, sleeping[i].id, sleeping[i].time, sleeping[i].next);
		//breakpoint();
	}
}



void loader_tick() {
	if (first_slept == FREE_PCB_PID) {
		return;
	}

	//breakpoint();

	sleeping[first_slept].time--;
	while(first_slept != FREE_PCB_PID && sleeping[first_slept].time == 0) {
		sched_unblock(first_slept);
		sleeping[first_slept].id = FREE_PCB_PID;
		first_slept = sleeping[first_slept].next;
	}
}

void loader_exit(void) {
	pid local_tmp_pid = sched_exit();

	debug_log("loader_exit() called");

	// TODO Refactor this shit
	pcb_t* task = &processes[local_tmp_pid];
	if (task->prev == FREE_PCB_PID && task->next == FREE_PCB_PID) {
		// nothing
	} else if (task->prev == FREE_PCB_PID && task->next != FREE_PCB_PID) {
		// top of the list
		processes[task->next].prev = FREE_PCB_PID;
	} else if (task->prev != FREE_PCB_PID && task->next != FREE_PCB_PID) {
		// man in the middle
		processes[task->next].prev = task->prev;
		processes[task->prev].next = task->next;
	} else {
		processes[task->prev].next = FREE_PCB_PID;
	}

	mm_dir_free((mm_page*) processes[local_tmp_pid].cr3);

	processes[local_tmp_pid].id = FREE_PCB_PID;
	tmp_pid = local_tmp_pid;
	task_switch();
}
