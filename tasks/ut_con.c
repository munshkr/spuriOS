#include <syscalls.h>
#include <fs.h>
#include <debug.h>

int main () {
	int fd = open("/console", FS_OPEN_RDWR);
	assert(fd == 0);

	int sz = write(fd, "\\c0dHello \\c0eworld!\n", 4096);
	assert(sz == 22);

	close(fd);
	return 0;
}
