#include <fs.h>
#include <lib.h>

static void run_task(const char* file);
static void run_unit_tests(void);

int main(void) {
	run_task("/disk/bin/console.pso");
	run_unit_tests();

	return 0;
}

static void run_unit_tests(void) {
	run_task("/disk/bin/getpid.pso");
	run_task("/disk/bin/palloc.pso");
	run_task("/disk/bin/cpuid.pso");
	run_task("/disk/bin/cp2user.pso");
	run_task("/disk/bin/ut_con.pso");
}

static void run_task(const char* file) {
	pid pd = run(file);
	if (pd < 0) {
		int con = open("/console", FS_OPEN_RDWR);

		switch (pd) {
		case -RUN_ERROR_OPENING:
			fprintf(con, "init: Error opening %s\n", file);
			break;
		case -RUN_INVALID_EXECUTABLE:
			fprintf(con, "init: %s is not a valid PSO file\n", file);
			break;
		case -RUN_ERROR_READING:
			fprintf(con, "init: Error reading %s\n", file);
			break;
		case -RUN_UNAVAILABLE_MEMORY:
			fprintf(con, "init: Not enough memory for %s\n", file);
			break;
		default:
			fprintf(con, "init: Error %d running %s\n", pd, file);
			break;
		}

		while (1) { sleep(10); };
		close(con);
	}
}
