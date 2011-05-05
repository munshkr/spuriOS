#include <syscalls.h>

int main () {
	int i = 0;
	char c;
	for (i = 0; i < 10; i++) {
		c = getch();
		printf("%d ", (uint_32) c);
	}
	printf("\n");

	return 0;
}
