#include <syscalls.h>
#include <i386.h>

int main () {
	uint_32* buf;

	palloc();
	palloc();
	palloc();
	palloc();
	buf = (uint_32*) palloc();

	int pid = fork();

	if (pid == 0) {
		*buf = 0xCABE;
		printf("Child writing at %p, reading %x\n", buf, *buf);
		buf = (uint_32*) 0x4000A000;
		*buf = 0;
	} else {
		*buf = 0xCACA;
		printf("Father writing %p, reading %x\n", buf, *buf);
		buf = (uint_32*) 0x4000A000;
		*buf = 0;
	}

	return 0;
}
