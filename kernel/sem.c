#include <sem.h>
#include <loader.h>

void sem_wait(sem_t* s) {
	if (s->vl == 0) {
		loader_enqueue(&s->q);
	} else {
		s->vl--;
	}
}

void sem_signal(sem_t* s) {
	if (s->q == -1) {
		s->vl++;
	} else {
		loader_unqueue(&s->q);
	}
}

void sem_broadcast(sem_t* s) {
	while(s->q != -1) {
		loader_unqueue(&s->q);
	}
	s->vl = 0;
}
