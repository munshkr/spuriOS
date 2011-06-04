#include <fs.h>
#include <lib.h>

void run_unit_tests(void) {
	run("/disk/bin/getpid.pso");
	run("/disk/bin/palloc.pso");
	run("/disk/bin/cpuid.pso");
	run("/disk/bin/cp2user.pso");
	run("/disk/bin/con.pso");
}

int main(void) {
	int pid = run("/disk/bin/console.pso");
	if (pid <= 0) {
		int con_fd = open("/console", FS_OPEN_RDWR);
		fprintf(con_fd, "Failed to load Spursh at /disk/bin/console.pso!\n");
	}

	//run_unit_tests();

	while (1) {};

	return 0;
}
