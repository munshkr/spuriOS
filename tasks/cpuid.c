#include <syscalls.h>
#include <fs.h>
#include <debug.h>

int main () {
	char* buf = (char*) palloc();
	int fd = open("/proc/cpuid", FS_OPEN_RD);
	assert(fd == 0);

	int sz = read(fd, buf, 4096);
	assert(sz > 0 && sz <= 4096);

	close(fd);

	fd = open("/proc/cpuid", FS_OPEN_RD);
	assert(fd == 0);

	sz = read(fd, buf, 4096);
	assert(sz > 0 && sz <= 4096);

	int fd1 = open("/proc/cpuid", FS_OPEN_RD);
	assert(fd1 == 1);

	sz = read(fd1, buf, 4096);
	assert(sz > 0 && sz <= 4096);

	close(fd);
	close(fd1);

	fd = open("/proc/cpuid", 0);
	assert(fd < 0);

	return 0;
}
