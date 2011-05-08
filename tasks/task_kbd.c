#include <syscalls.h>

int main () {
	int pid = getpid();

	printf("\\c05[%u]\\c07 And I am task_kbd, always waiting for you to play with the keyboard...\n", pid);

	int i = 0;
	char sc = 0;
	for (i = 0; i < 20; i++) {
		sc = getsc();
		printf("\\c05[%u]\\c07 Scancode: %d\n", pid, sc);
	}

	printf("\\c05[%u]\\c07 After 20 keystrokes, I die here.\n", pid);
	return 0;
}
