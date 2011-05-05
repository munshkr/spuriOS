#include <syscalls.h>

int main () {
	int i = 0;
	char c = 0;
	for (i = 0; i < 10; i++) {
		c = getch();
//		__asm __volatile ("xchg %%bx, %%bx" : : );
		printf("%d ", (uint_32) c);
	}
	printf("\n");

	return 0;
}
