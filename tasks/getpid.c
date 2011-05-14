#include <syscalls.h>
#include <debug.h>

int main(void) {
	int pid = getpid();
	assert(pid > 0);
	return 0;
}
