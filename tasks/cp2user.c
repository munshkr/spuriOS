#include <syscalls.h>
#include <fs.h>
#include <debug.h>

int main () {
	int fd = open("/proc/cpuid", FS_OPEN_RDONLY);
	char* buggy = (char*) 0x1000;

	int sz = read(fd, buggy, 4096);
	assert(sz < 0);

	buggy = (char*) 0xEEEEEEEE;
	sz = read(fd, buggy, 4096);
	assert(sz < 0);

	close(fd);

	return 0;
}
