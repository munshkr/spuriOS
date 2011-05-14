#include <syscalls.h>
#include <fs.h>

int main () {
	int i;
	for (i = 0; i < 4; i++) {
		int fd = open("/proc/cpuid", FS_OPEN_RDONLY);
		printf("fd = %d\n", fd);
	}

	return 0;
}
