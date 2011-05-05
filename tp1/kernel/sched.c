#include <idt.h>
#include <sched.h>
#include <loader.h>

#define SCHED_QUANTUM_DEFAULT 10

typedef struct str_sched_task {
	pid pd;
	uint_32 quantum;
	struct str_sched_task* next;
	struct str_sched_task* prev;
} sched_task;

sched_task tasks[MAX_PID];
sched_task* actual;
sched_task* first;
sched_task* last;
int running_tasks;

void print_queue(void) {
	int i;
	for (i = 0; i < 3; i++) {
		vga_printf("index %u\n", i);
		vga_printf("\tpd = %u\n", tasks[i].pd);
		vga_printf("\tquantum = %u\n", tasks[i].quantum);
		if (tasks[i].prev != NULL) {
			vga_printf("\tprev_id = %u\n", tasks[i].prev->pd);
		} else {
			vga_printf("\tprev_id = NULL\n");
		}
		if (tasks[i].next != NULL) {
			vga_printf("\tnext_id = %u\n", tasks[i].next->pd);
		} else {
			vga_printf("\tnext_id = NULL\n");
		}
	}
	if (actual != NULL) {
		vga_printf("actual_id = %u\t", actual->pd);
	} else {
		vga_printf("actual_id = NULL\t");
	}
	if (first != NULL) {
		vga_printf("first_id = %u\t", first->pd);
	} else {
		vga_printf("first_id = NULL\t");
	}
	if (last != NULL) {
		vga_printf("last_id = %u\n", last->pd);
	} else {
		vga_printf("last_id = NULL\n");
	}
	//breakpoint();
}

void sched_init(void) {
	memset(tasks, 0, sizeof(sched_task) * MAX_PID);
	actual = NULL;
	first = NULL;
	last = NULL;
	running_tasks = 0;
}

void sched_load(pid pd) {
	int i;
	for (i = 0 ; i < MAX_PID ; i++) {
		if (tasks[i].pd == 0) {
			break;
		}
	}
	kassert_verbose(i != MAX_PID, "Scheduler run out of task slots!!!");
	//kassert(pd < MAX_PID);

	vga_printf("before sched_load()\n");
	print_queue();

	sched_task* tmp_task = &tasks[i];
	tasks[i].pd = pd;
	tasks[i].quantum = SCHED_QUANTUM_DEFAULT;
	if (running_tasks == 0) {
		tasks[i].next = tmp_task;
		tasks[i].prev = tmp_task;
		last = tmp_task;
		first = tmp_task;
	} else {
		tasks[i].next = first;
		tasks[i].prev = last;
		last->next = tmp_task;
		if (last->prev == last) {
			last->prev = tmp_task;
		}
		last = tmp_task;
	}
	running_tasks++;

	vga_printf("after sched_load()\n");
	print_queue();
}

void sched_unblock(pid pd) {
	sched_load(pd);
}

int sched_exit() {
	if (running_tasks > 1) {
		sched_task* tmp_next = actual->next;
		(actual->prev)->next = actual->next;
		(actual->next)->prev = actual->prev;
		memset(actual, 0, sizeof(sched_task));
		actual = tmp_next;
	} else {
		memset(actual, 0, sizeof(sched_task));
		first = NULL;
		last = NULL;
		actual = NULL;
	}
	running_tasks--;
	vga_printf("sched_exit()\n");
	print_queue();
	// FIXME We should have a cur_pid here, instead of using loader's cur_pid
	return cur_pid;
}

int sched_block() {
	return sched_exit();
}

int sched_tick() {
	pid pd;
	if (actual == NULL) {
		if (running_tasks <= 0) {
			pd = 0;
		} else {
			actual = first;
			pd = actual->pd;
		}
	} else {
		kassert(actual->quantum >= 0);
		if (actual->quantum > 0) {
			actual->quantum--;
			pd = actual->pd;
		} else {
			actual->quantum = SCHED_QUANTUM_DEFAULT;
			actual = actual->next;
			pd = actual->pd;
		}
	}
	return pd;
}
