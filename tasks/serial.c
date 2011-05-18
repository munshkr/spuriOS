#include <syscalls.h>
#include <fs.h>

int main () {
	uint_32 com = open("/serial0", FS_OPEN_RDWR);
	uint_32 tty = open("/console", FS_OPEN_RDWR);

	char buf[2];
	while (1) {
		read(com, (char*) &buf, 4096);
		write(tty, buf, 4096);
	}

	write(com, "buenos dias", 11);

	close(tty);
	close(com);

	return 0;
}
