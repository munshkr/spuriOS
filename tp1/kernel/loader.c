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

static void timer_handler(registers_t* regs);

#define FREE_PCB_PID 0xFFFFFFFF
#define FREE_QUEUE -1
#define USER_MEMORY_START 0x400000

pcb_t processes[MAX_PID];
pid cur_pid;

inline void be_task() {
	ltr(SS_TSS);
}

inline void initialize_process_list() {
	uint_32 id;
	for (id = 0; id < MAX_PID; id++) {
		processes[id].id = FREE_PCB_PID;
	}
}

void loader_init(void) {
	debug_log("initializing loader");

	idt_register(ISR_IRQ0, timer_handler, PL_KERNEL);
	timer_init(1500);

	initialize_process_list();

	be_task();
	cur_pid = IDLE_PID;
	processes[IDLE_PID].id = IDLE_PID;
	processes[IDLE_PID].privilege_level = PL_KERNEL;
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

static inline void* get_frame_from_page(void* virt_addr) {
	// TODO
	// Devuelve la dirección física que tiene mapeada
	// la página dada por parámetro (en el PDT actual).
	return NULL;
}

static inline void map_frame(void* phys_addr, void* virt_addr, mm_page* pdt) {
	// TODO
	// Mapea un frame en phys_addr a la pagina pedida (virt_addr) en la pdt.
	// Define tabla de pagina y la inicializa, si es necesario.
}

static inline void unmap_page(void* virt_addr, mm_page* pdt) {
	// TODO
	// Setear en 0 la pagina pedida en la pdt.
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

	arch_state_t* state = &processes[pid].arch_state;
	state->eflags = TASK_DEFAULT_EFLAGS;
	state->eax = state->ebx = state->ecx = state->edx = state->esi = state->edi = 0;	// optional

	// Set code and data segments depending on the parameter `pl`
	if (pl == PL_USER) {
		state->code_segment = SS_U_CODE;
		state->data_segment = SS_U_DATA;
	} else if (pl == PL_KERNEL) {
		state->code_segment = SS_K_CODE;
		state->data_segment = SS_K_DATA;
	}

	state->eip = (uint_32) f->_main;

	// Create new Page Directory Table
	mm_page* new_pdt = mm_dir_new();
	state->cr3 = (uint_32) new_pdt;

	// Calculate number of pages required for the new task
	uint_32 total_pages = (f->mem_end - f->mem_start) / 4;
	if ((f->mem_end - f->mem_start) % 4) total_pages++;

	// Temporal page to map new frames and initialize them
	void* temp_page;
	void* old_frame = NULL;
	if (cur_pid == PID_IDLE_TASK) {
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
		old_frame = get_frame_from_page(temp_page);
	}

	mm_page* current_pdt = (mm_page*) rcr3();
	void* task_mem_p = (sint_8*) f->mem_start;
	void* task_data_p = (sint_8*) f->data;

	int i;
	for (i = 0; i < total_pages; ++i) {
		void* frame = mm_mem_alloc();

		// Map `frame` in the new Page Directory Table
		map_frame(frame, task_mem_p, new_pdt);

		// Temporally map `frame` to initialize it
		map_frame(frame, temp_page, current_pdt);

		// Copy task code from memory or initialize with zeros the new frames
		if (task_mem_p >= (void*) f->mem_end_disk) {
			memset(temp_page, 0, PAGE_SIZE);
		} else {
			size_t bytes = MIN(PAGE_SIZE, (void*) f->mem_end_disk - task_mem_p);
			memcpy(task_data_p, temp_page, bytes);
			memset(temp_page, 0, PAGE_SIZE - bytes);
		}

		task_mem_p  += PAGE_SIZE;
		task_data_p += PAGE_SIZE;
	}

	unmap_page(temp_page, current_pdt);

	// Restore original frame used by current task
	if (cur_pid != PID_IDLE_TASK) {
		map_frame(old_frame, temp_page, current_pdt);
	}

	// Create new stack frame
	void* stack_frame = mm_mem_alloc();
	map_frame(stack_frame, (void*) TASK_STACK_ADDRESS, new_pdt);
	state->esp = state->ebp = TASK_STACK_TOP_ADDRESS;

	//sched_load(pid);

	return pid;
}

// TODO: check if this should work with a mutex or at least with an STI (stop interrupts)
void loader_enqueue(int* cola) {
	pid tmp_pid;
	if (*cola == FREE_QUEUE) {
		*cola = cur_pid;
	} else {
		tmp_pid = *cola;
		while(processes[tmp_pid].next != FREE_PCB_PID) {
			tmp_pid = processes[tmp_pid].next;
		}
		processes[tmp_pid].next = cur_pid;
		processes[cur_pid].prev = tmp_pid;
	}
	sched_block();
}

// TODO: check if this should work with a mutex or at least with an STI (stop interrupts)
void loader_unqueue(int* cola) {
	pid tmp_pid;
	if (*cola != FREE_QUEUE) {
		tmp_pid = processes[*cola].next;
		processes[tmp_pid].prev = FREE_PCB_PID;
		processes[*cola].next = FREE_PCB_PID;
		sched_unblock(*cola);
		*cola = tmp_pid;
	}
}

void loader_exit(void) {
}

inline void save_arch_state_to(pcb_t* pcb, registers_t* regs) {
	arch_state_t* dest = &(pcb->arch_state);

	dest->eax = regs->eax;
	dest->ebx = regs->ebx;
	dest->ecx = regs->ecx;
	dest->edx = regs->edx;
	dest->esi = regs->esi;
	dest->edi = regs->edi;

	dest->ebp = regs->ebp;
	dest->esp = regs->esp;

	dest->eip = regs->eip;
	dest->eflags = regs->eflags;

	dest->cr3 = rcr3();
}

inline void restore_arch_state_from(pcb_t* pcb, registers_t* regs, bool needs_segment_change) {
	arch_state_t* source = &(pcb->arch_state);

	regs->eax = source->eax;
	regs->ebx = source->ebx;
	regs->ecx = source->ecx;
	regs->edx = source->edx;
	regs->esi = source->esi;
	regs->edi = source->edi;

	regs->ebp = source->ebp;
	regs->esp = source->esp;

	regs->eip = source->eip;
	regs->eflags = source->eflags;

	if (needs_segment_change) {
		// If stack change happened
		if (SS_PL(regs->cs) != PL_KERNEL) {
			regs->user_ss = source->data_segment;
			regs->user_esp = source->esp;
		} else {
			lss(source->data_segment);
		}
		lds(source->data_segment);
		les(source->data_segment);
	}

	lcr3(source->cr3);
}

void loader_switchto(pid to_id, registers_t* regs) {
	kassert(processes[to_id].id != FREE_PCB_PID);

	if (cur_pid != to_id) {
		pcb_t* pcb_cur = &processes[cur_pid];
		save_arch_state_to(pcb_cur, regs);

		pcb_t* pcb_dest = &processes[to_id];
		bool needs_segment_change =
			pcb_cur->privilege_level != pcb_dest->privilege_level;

		restore_arch_state_from(pcb_dest, regs, needs_segment_change);

		cur_pid = to_id;
	}
}

static void timer_handler(registers_t* regs) {
	tick++;
//	timer_draw_clock();

	pid next_process = sched_tick();
	if (next_process != cur_pid) {
		vga_printf("switching to %d\n", next_process);
		loader_switchto(next_process, regs);
	}
}

