#include <syscalls.h>
#include <fs.h>
#include <debug.h>
#include <lib.h>

int main () {

	int con_fd = open("/console", FS_OPEN_RDWR);

	char* buf = (char*) palloc();

	fprintf(con_fd, "\n\\c0F:/\\c07open(\"/disk/\");\n");

	int fd = open("/disk/", FS_OPEN_RD);
	read(fd, buf, 4096);
	fprintf(con_fd, "%s\n", buf);

	fprintf(con_fd, "\n\\c0F:/\\c07open(\"/disk/lost+found\");\n");

	fd = open("/disk/lost+found", FS_OPEN_RD);
	read(fd, buf, 4096);
	fprintf(con_fd, "%s\n", buf);

	while(TRUE) {
	}

	close(fd);
	close(con_fd);

	return 0;
}

