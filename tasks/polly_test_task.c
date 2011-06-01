#include <syscalls.h>
#include <fs.h>
#include <debug.h>

int main () {
	int fd = open("/disk/", FS_OPEN_RD);
	if (fd);
	return 0;
}

