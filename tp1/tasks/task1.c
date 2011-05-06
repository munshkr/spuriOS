int data[10];

#include <syscalls.h>

int main(void) {
	int pid = getpid();

	printf("\\c06[%u]\\c07 Hello! I am task1, and I sleep. A lot.\n", pid);
	sleep(6000);

	while (1) {
		printf("\\c06[%u]\\c07 Gone to sleep\n", pid);
		sleep(3000);
		printf("\\c06[%u]\\c07 Woke up\n", pid);

		sleep(2000);
		uint_32* page = palloc();
		printf("\\c06[%u]\\c07 Called palloc(), got a new page at %p\n", pid, page);

		*page = 0xCAFE;
		printf("\\c06[%u]\\c07 Written 0xCAFE to new page, and I can read it too: %x\n", pid, *page);
	}

	return 0;
}
