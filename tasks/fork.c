#include <syscalls.h>
#include <i386.h>
#include <fs.h>
#include <lib.h>

int main () {

	int fd = open("/console", FS_OPEN_RDWR);

	unsigned int pid = fork();

	if (pid == 0) {
		fprintf(fd, "Proceso hijo!\n");
	} else {
		fprintf(fd, "Padre, pid del hijo %d\n", pid);
	}

	while (1) ;

}
