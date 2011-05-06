#include <syscalls.h>

int main () {
	int i = 0;
	char sc = 0;
	for (i = 0; i < 10; i++) {
		sc = getsc();
		printf("(PID %d) Scancode = %d\n", getpid(), (uint_32) sc);
	}
	printf("\n");

	return 0;
}
