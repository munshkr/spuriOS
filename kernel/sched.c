#include <idt.h>
#include <sched.h>
#include <loader.h>
#include <lib.h>

#define SCHED_QUANTUM_DEFAULT 10
#define FREE_PID -1

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

void print_queue(unsigned int max) {
	kassert(max < MAX_PID);
	int i;
	for (i = 0; i < max; i++) {
		vga_printf("index %u\n", i);
		vga_printf("\tpd = %d\n", tasks[i].pd);
		vga_printf("\tquantum = %u\n", tasks[i].quantum);
		if (tasks[i].prev != NULL) {
			vga_printf("\tprev_pid = %d\n", tasks[i].prev->pd);
		} else {
			vga_printf("\tprev_pid = NULL\n");
		}
		if (tasks[i].next != NULL) {
			vga_printf("\tnext_pid = %d\n", tasks[i].next->pd);
		} else {
			vga_printf("\tnext_pid = NULL\n");
		}
		breakpoint();
	}
	if (actual != NULL) {
		vga_printf("actual_pid = %d\t", actual->pd);
	} else {
		vga_printf("actual_pid = NULL\t");
	}
	if (first != NULL) {
		vga_printf("first_pid = %d\t", first->pd);
	} else {
		vga_printf("first_pid = NULL\t");
	}
	if (last != NULL) {
		vga_printf("last_pid = %d\n", last->pd);
	} else {
		vga_printf("last_pid = NULL\n");
	}
	breakpoint();
}

void sched_init(void) {
	memset(tasks, 0, sizeof(sched_task) * MAX_PID);
	actual = NULL;
	first = NULL;
	last = NULL;
	running_tasks = 0;

	int i;
	for (i = 0; i < MAX_PID; i++) {
		tasks[i].pd = FREE_PID;
	}
}

void sched_load(pid pd) {
	kassert(pd >= 0 && pd < MAX_PID);

	int i;
	for (i = 0 ; i < MAX_PID ; i++) {
		if (tasks[i].pd == FREE_PID) {
			break;
		}
	}
	kassert_verbose(i != MAX_PID, "Scheduler run out of task slots!!!");

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
	first->prev = last;
	running_tasks++;
}

void sched_unblock(pid pd) {
	sched_load(pd);
}

int sched_exit() {
	kassert(running_tasks > 0);

	pid pd;

	if (running_tasks > 1) {
		if (first->pd == actual->pd) {
			first = actual->next;
		}
		if (last->pd == actual->pd) {
			last = actual->prev;
		}
		sched_task* tmp_next = actual->next;
		(actual->prev)->next = actual->next;
		(actual->next)->prev = actual->prev;
		memset(actual, 0, sizeof(sched_task));
		sched_task* old = actual;
		actual = tmp_next;
		pd = actual->pd;
		old->pd = FREE_PID;
	} else {
		memset(actual, 0, sizeof(sched_task));
		actual->pd = FREE_PID;
		first = NULL;
		last = NULL;
		actual = NULL;
		pd = 0;
	}
	running_tasks--;

	return pd;
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
			kassert(actual->next != NULL);
			actual = actual->next;
			pd = actual->pd;
		}
	}
	//print_queue(4);
	//breakpoint();
	return pd;
}
