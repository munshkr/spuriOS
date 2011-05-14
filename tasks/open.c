#include <syscalls.h>
#include <fs.h>

int main () {
	char* buf = (char*) palloc();
	int fd = open("/proc/cpuid", FS_OPEN_RDONLY);

	char* tmp = buf;
	int i = 0, sz = 0;
	for (i = 0; i < 5; i++) {
		sz = read(fd, tmp, 10);
		tmp += sz;
		printf("read %d from fd %d\n", sz, fd);
	}
	printf("%s\n", buf);

	close(fd);

	buf = (char*) palloc();
	fd = open("/proc/cpuid", FS_OPEN_RDONLY);

	printf("read %d from fd %d\n", read(fd, buf, 4096), fd);
	printf("%s\n", buf);

	return 0;
}
