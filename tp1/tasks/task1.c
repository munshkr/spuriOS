int data[10];

#include <syscalls.h>

int main(void) {
	int i;
	while (1) {
		printf("Hello world! I'm PID = %d\n", getpid());
		for (i = 0; i < 10000000; i++); // DELAY
	}

	return 0;
}
