int data[10];

#include <syscalls.h>

int main(void) {
	while (1) {
		printf("Hello world! I'm PID = %d\n", getpid());
		sleep(3000);
	}

	return 0;
}
