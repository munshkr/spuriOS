#include <syscalls.h>
#include <debug.h>

int main () {
	uint_32* pg;
	uint_32 i;

	for (i = 0; i < 20; i++) {
		pg = (uint_32*) palloc();
		*pg = 0xCAFE;
		assert(*pg = 0xCAFE);
	}

	return 0;
}
