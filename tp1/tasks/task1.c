int data[10];

#include <syscalls.h>

int main(void) {
	unsigned int j = 0, i = 0;
	while (j < 120) {
		printf("Hello world! I'm PID = %d\n", getpid());
		for (i = 0; i < 1000000000; i++); // DELAY
		j++;
	}

	return 0;
}
