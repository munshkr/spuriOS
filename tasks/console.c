#include <syscalls.h>
#include <fs.h>
#include <i386.h>

int main () {
	int fd = open("/console", FS_OPEN_RDWR);

	while (1) {
		char sc = 0;
		read(fd, &sc, 1);
		write(fd, &sc, 1);
	}

	close(fd);

	return 0;
}
