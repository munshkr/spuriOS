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
		last = tmp_task;
	}
	running_tasks++;
}

void sched_unblock(pid pd) {
	sched_load(pd);
}

int sched_exit() {
	pid pd;
	if (running_tasks > 1) {
		sched_task* tmp_next = actual->next;
		(actual->prev)->next = actual->next;
		(actual->next)->prev = actual->prev;
		memset(actual, 0, sizeof(sched_task));
		actual = tmp_next;
		pd = actual->pd;
	} else {
		memset(actual, 0, sizeof(sched_task));
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
			actual = actual->next;
			pd = actual->pd;
		}
	}
	return pd;
}
