#include <loader.h>
#include <timer.h>
#include <idt.h>
#include <lib.h>
#include <i386.h>
#include <debug.h>
#include <gdt.h>
#include <sched.h>
#include <vga.h>
#include <mm.h>
#include <fs.h>
#include <tipos.h>
#include <device.h>

const char PSO_SIGNATURE[4] = "PSO";

extern void (timer_handler)();
extern void (task_init)();
extern void (task_switch)();

pcb_t processes[MAX_PID];
pid cur_pid;
pid tmp_pid;

slept_task sleeping[MAX_PID];
pid first_slept;

mm_page* cur_pdt() {
	return (mm_page*) processes[cur_pid].cr3;
}

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

void loader_init(void) {
	debug_log("initializing loader");

	idt_register_asm(ISR_IRQ0, timer_handler, PL_KERNEL);
	timer_init(1193);

	initialize_process_list();

	initialize_sleeping_list();

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

static inline pid get_new_pid() {
	int pid;
	for (pid = 0; pid < MAX_PID; pid++) {
		if (processes[pid].id == FREE_PCB_PID) break;
	}
	// No more free PIDs! (consider reducing load or enlarging MAX_PID...)
	kassert(pid < MAX_PID);
	return pid;
}

uint_32 data_pages_needed_for(pso_file* f) {
	uint_32 pages = (f->mem_end - f->mem_start) / PAGE_SIZE;
	if ((f->mem_end - f->mem_start) % PAGE_SIZE) pages++;
	return pages;
}

#define STACK_TOP 1023
inline void create_kernel_stack(uint_32* stack, uint_32 eflags, uint_32 eip) {
	stack[STACK_TOP] = eflags;
	stack[STACK_TOP - 1] = SS_K_CODE; // CS
	stack[STACK_TOP - 2] = eip;
	stack[STACK_TOP - 3] = (uint_32) task_init; // task_swith ret EIP
	stack[STACK_TOP - 8] = (uint_32)(TASK_K_STACK_ADDRESS + (STACK_TOP - 4)  * 4); // ESP
	stack[STACK_TOP - 9] = (uint_32)(TASK_K_STACK_ADDRESS + (STACK_TOP - 1) * 4); // EBP
}

inline void create_user_stack(uint_32* stack, uint_32 eflags, uint_32 eip, uint_32 user_esp) {
	stack[STACK_TOP] = SS_U_DATA | PL_USER; // SS (USER)
	stack[STACK_TOP - 1] = user_esp;
	stack[STACK_TOP - 2] = eflags;
	stack[STACK_TOP - 3] = SS_U_CODE | PL_USER; // CS (USER)
	stack[STACK_TOP - 4] = eip;
	stack[STACK_TOP - 5] = (uint_32) task_init; // task_swith ret EIP
	stack[STACK_TOP - 10] = (uint_32)(TASK_K_STACK_ADDRESS + 1017 * 4); // ESP
	stack[STACK_TOP - 11] = (uint_32)(TASK_U_STACK_ADDRESS + 1024 * 4); // EBP (USER)
}

#define K_IN_SWITCH_ESP ((uint_32)(TASK_K_STACK_ADDRESS + 1012 * 4))
#define U_IN_SWITCH_ESP ((uint_32)(TASK_K_STACK_ADDRESS + 1010 * 4))
#define U_STACK_BOTTOM ((uint_32)(TASK_U_STACK_ADDRESS + 1024 * 4))
inline void create_stacks_for(pso_file* f, mm_page* current_pdt, mm_page* new_pdt,
	uint_32 pl, void* temp_page, pcb_t* new_proc) {

	void* kernel_stack_frame = mm_mem_alloc();
	mm_map_frame(kernel_stack_frame, (void*) TASK_K_STACK_ADDRESS, new_pdt, PL_KERNEL);

	mm_map_frame(kernel_stack_frame, temp_page, current_pdt, PL_KERNEL);
	memset(temp_page, 0, PAGE_SIZE);

	if (pl == PL_KERNEL) {
		create_kernel_stack((uint_32*) temp_page, TASK_DEFAULT_EFLAGS, (uint_32) f->_main);
		new_proc->esp = K_IN_SWITCH_ESP;
	} else {
		void* user_stack_frame = mm_mem_alloc();
		mm_map_frame(user_stack_frame, (void*) TASK_U_STACK_ADDRESS, new_pdt, PL_USER);
		create_user_stack((uint_32*) temp_page, TASK_DEFAULT_EFLAGS, (uint_32) f->_main, U_STACK_BOTTOM);
		new_proc->esp = U_IN_SWITCH_ESP;
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

		*(uint_32**)old_frame = mm_frame_from_page(*temp_page, current_pdt);
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
	pid pid = get_new_pid();

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
	kassert(processes[cur_pid].prev == FREE_QUEUE);
	kassert(processes[cur_pid].next == FREE_QUEUE);

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

	if (first_slept == FREE_PCB_PID) {
		first_slept = cur_pid;
		sleeping[cur_pid].id = cur_pid;
		sleeping[cur_pid].time = time;
		sleeping[cur_pid].next = FREE_PCB_PID;
	} else {
		tmp_slept = first_slept;
		while (tmp_slept != FREE_PCB_PID) {
			if (sleeping[tmp_slept].time < time) {
				time -= sleeping[tmp_slept].time;
			} else {
				break;
			}
			prev_slept = tmp_slept;
			tmp_slept = sleeping[tmp_slept].next;
		}
		if (tmp_slept == FREE_PCB_PID) {
			sleeping[prev_slept].next = cur_pid;
			sleeping[cur_pid].id = cur_pid;
			sleeping[cur_pid].time = time;
			sleeping[cur_pid].next = FREE_PCB_PID;
		} else {
			if (prev_slept == FREE_PCB_PID) {
				first_slept = cur_pid;
			} else {
				sleeping[prev_slept].next = cur_pid;
			}
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

	device_close_fds_for(cur_pid);

	// TODO Refactor this shit
	pcb_t* task = &processes[cur_pid];
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

	mm_dir_free((mm_page*) processes[cur_pid].cr3);

	processes[cur_pid].id = FREE_PCB_PID;
	tmp_pid = local_tmp_pid;
	task_switch();
}

void free_run_resources(void* start_vaddr, uint_32 page_count, fd_t fd) {
	mm_page* pdt = cur_pdt();

	uint_32 i = 0;
	void* cur_vaddr = start_vaddr;

	for (i = 0; i < page_count; i++) {
		void* frame = mm_frame_from_page(cur_vaddr, pdt);
		mm_unmap_page(cur_vaddr, pdt);
		mm_mem_free(frame);
		cur_vaddr += PAGE_SIZE;
	}
	mm_free_page_table_for(start_vaddr, pdt);
	close(fd);
}

pid run(const char* filename) {
	fd_t fd = open(filename, FS_OPEN_RD);
	if (fd < 0) {
		return -RUN_ERROR_OPENING;
	}

	void* start_vaddr = mm_vaddr_for_free_pdt_entry(cur_pdt());

	void* frame = mm_mem_alloc();
	if (!frame) {
		free_run_resources(0, 0, fd);
		return -RUN_UNAVAILABLE_MEMORY;
	}

	mm_map_frame(frame, start_vaddr, cur_pdt(), PL_USER);
	void* copy_pointer = start_vaddr;

	uint_32 readed = 0;
	while (readed < sizeof(pso_file)) {
		uint_32 actually_readed = read(fd, copy_pointer, sizeof(pso_file) - readed);

		if (actually_readed > 0) {
			copy_pointer += actually_readed;
			readed += actually_readed;
		} else {
			free_run_resources(start_vaddr, 1, fd);
			return -RUN_ERROR_READING;
		}
	}

	if (strncmp((const char*) ((pso_file*) start_vaddr)->signature, PSO_SIGNATURE, 4)) {
		free_run_resources(start_vaddr, 1, fd);
		return -RUN_INVALID_EXECUTABLE;
	}

	void* cur_vaddr = start_vaddr;

	uint_32 effective_bytes = ((pso_file*) start_vaddr)->mem_end_disk - ((pso_file*) start_vaddr)->mem_start;

	uint_32 needed_pages = effective_bytes / PAGE_SIZE;
	if (effective_bytes % PAGE_SIZE) needed_pages++;

	needed_pages--; // We already request one page

	uint_32 i;
	for (i = 0; i < needed_pages; i++) {
		frame = mm_mem_alloc();
		cur_vaddr += PAGE_SIZE;
		if (!frame) {
			free_run_resources(start_vaddr, i + 1, fd);
			return -RUN_UNAVAILABLE_MEMORY;
		}

		mm_map_frame(frame, cur_vaddr, cur_pdt(), PL_USER);
	}

	// READ
	while (readed < effective_bytes) {
		uint_32 actually_readed = read(fd, copy_pointer, effective_bytes - readed);

		if (actually_readed > 0) {
			copy_pointer += actually_readed;
			readed += actually_readed;
		} else {
			free_run_resources(start_vaddr, needed_pages + 1, fd);
			return -RUN_ERROR_READING;
		}
	}

	pid pid = loader_load((pso_file*) start_vaddr, PL_USER);
	free_run_resources(start_vaddr, needed_pages + 1, fd);

	return pid;
}

// TODO MM Function (?)
static void copy_nonkernel_pages(mm_page* father_pdt, mm_page* child_pdt) {
	uint_32 pd_entry, pt_entry;
	for (pd_entry = 1; pd_entry < 1024; pd_entry++) { // Starts at 4MB
		if (father_pdt[pd_entry].attr & MM_ATTR_P) {

			mm_page* table = (mm_page*)(father_pdt[pd_entry].base << 12);
			for (pt_entry = 0; pt_entry < 1024; pt_entry++) {
				void* virtual = (void*)((pd_entry << 22) + (pt_entry << 12));
				if (table[pt_entry].attr & MM_ATTR_P) {

					// We do not copy this page here
					if (virtual == (void*) TASK_K_STACK_ADDRESS) continue;

					if (table[pt_entry].attr & MM_ATTR_USR_SHARED) {
						void* frame = (void*) (table[pt_entry].base << 12);
						// FIXME mm_map_frame should accept attributes as a parameter
						mm_map_frame(frame, virtual, child_pdt,
							(table[pt_entry].attr & MM_ATTR_US_U ? PL_USER : PL_KERNEL));
						mm_pt_entry_for(virtual, child_pdt)->attr |= MM_ATTR_USR_SHARED;
					} else {
						void* frame = (void*) (table[pt_entry].base << 12);
						mm_page* tmp_entry_ptr;

						mm_map_frame(frame, virtual, child_pdt,
							(table[pt_entry].attr & MM_ATTR_US_U ? PL_USER : PL_KERNEL));

						// Table entry for child
						tmp_entry_ptr = mm_pt_entry_for(virtual, child_pdt);
						tmp_entry_ptr->attr |= MM_ATTR_USR_COR;
						tmp_entry_ptr->attr &= ~MM_ATTR_RW;

						// Table entry for parent
						tmp_entry_ptr = &table[pt_entry];
						tmp_entry_ptr->attr |= MM_ATTR_USR_COR;
						tmp_entry_ptr->attr &= ~MM_ATTR_RW;
					}
				} else {
					mm_set_pt_entry(virtual, ((uint_32*) table)[pt_entry], child_pdt);
				}
			}
		}
	}

}

static void create_child_kernel_stack(mm_page* father_pdt, mm_page* child_pdt) {
	void* frame = mm_mem_alloc();
	mm_map_frame(frame, (void*) TASK_K_STACK_ADDRESS, child_pdt, PL_KERNEL);

	uint_32* stack = (uint_32*) TASK_K_STACK_ADDRESS;
	uint_32 eip, eflags, user_esp;

	eip = stack[STACK_TOP - 4];
	eflags = stack[STACK_TOP - 2];
	user_esp = stack[STACK_TOP - 1];

	mm_map_frame(frame, MM_TMP_PAGE, father_pdt, PL_KERNEL);
	memset(MM_TMP_PAGE, 0, PAGE_SIZE);
	create_user_stack((uint_32*) MM_TMP_PAGE, eflags, eip, user_esp);
	mm_unmap_page(MM_TMP_PAGE, father_pdt);
}

pid fork() {
	// Current process cannot be in any waiting queue
	kassert(processes[cur_pid].prev == FREE_QUEUE &&
		processes[cur_pid].next == FREE_QUEUE);

	// Current process cannot be a kernel process
	kassert(processes[cur_pid].privilege_level == PL_USER);

	pid child = get_new_pid();
	mm_page* child_pdt = mm_dir_new();

	processes[child] = (pcb_t) {
		.id = child,
		.cr3 = (uint_32) child_pdt,

		.prev = FREE_QUEUE,
		.next = FREE_QUEUE,

		.privilege_level = processes[cur_pid].privilege_level,
		.esp = (processes[cur_pid].privilege_level == PL_KERNEL ?
			K_IN_SWITCH_ESP : U_IN_SWITCH_ESP),
		.next_empty_page_addr = processes[cur_pid].next_empty_page_addr
	};

	copy_nonkernel_pages(cur_pdt(), child_pdt);
	create_child_kernel_stack(cur_pdt(), child_pdt);
	device_copy_fds(cur_pid, child);

	sched_load(child);

	return child;
}

pid getpid() {
	return cur_pid;
}

void exit() {
	loader_exit();
}

void sleep(registers_t* regs) {
	// EBP (+0), EIP (+4), time (+8)
	uint_32 time = *(uint_32*)(regs->user_esp + 8);
	loader_sleep(time);
}
