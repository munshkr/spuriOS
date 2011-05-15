#include <syscalls.h>
#include <fs.h>

int main () {
	int fd = open("/console", FS_OPEN_RDWR);

	sleep(2000);

	close(fd);

	return 0;
}
