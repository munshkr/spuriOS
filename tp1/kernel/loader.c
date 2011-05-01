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
#define DEFAULT_EFLAGS 0x2

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
	state->eax = state->ebx = state->ecx = state->edx = state->esi = state->edi = 0;
	state->eflags = DEFAULT_EFLAGS;

	if (pl == PL_USER) {
		state->code_segment = SS_U_CODE;
		state->data_segment = SS_U_DATA;
	} else if (pl == PL_KERNEL) {
		state->code_segment = SS_K_CODE;
		state->data_segment = SS_K_DATA;
	}

	state->eip = (uint_32) f->_main;

	// FIXME mm_dir_new() causes a GP fault!!
	//mm_page* new_pdt = mm_dir_new();
	//state->cr3 = (uint_32) new_pdt;

	// Request new frames for code/data
	uint_32 total_pages = (f->mem_end - f->mem_start) / 4;
	if ((f->mem_end - f->mem_start) % 4) total_pages++;

	int i;
	for (i = 0; i < total_pages; ++i) {
		//mm_page* page = mm_mem_alloc();
		// Asignar `page` a la tabla de paginas y directorio
		// correspondientes en new_pdt;
		// ...

		// Si corresponde, copiar el codigo de pso_file
		// hasta donde marca mem_end_disk
		// ...

		// Si nos pasamos de mem_end_disk, inicializar
		// la memoria con 0.
		// ...
	}

	// TODO New stack
	//state->esp = state->ebp = ...

	//sched_load(pid);

	return pid;
}

void loader_enqueue(int* cola) {
}

void loader_unqueue(int* cola) {
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

