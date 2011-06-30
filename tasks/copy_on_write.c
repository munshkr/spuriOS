#include <syscalls.h>
#include <i386.h>
#include <fs.h>
#include <lib.h>

#define PAGE_SIZE 4096
#define BUFFER_PAGES 4    // max 1024 pages

static int con = 0;
static sem_t mutex;

static int init_shared_page(void** page);
static void cleanup();

int main () {
	int res = 0;
	con = open("/console", FS_OPEN_RDWR);
	if (con < 0) {
		return -1;
	}

	fprintf(con, "Test copy-on-write memory usage.\n");

	char* shared_page;
	int* sz;
	init_shared_page((void**)&shared_page);
	init_shared_page((void**)&sz);

	char* private_page = palloc();

	int mi_id = 0;

	int cpuid = 0;

	cpuid = open("/proc/cpuid", FS_OPEN_RDWR);
	*sz = read(cpuid, shared_page, PAGE_SIZE);
	fprintf(con, "\\c0BAntes del fork: \\c0F%s\n", shared_page);
	close(cpuid);

	sem_open(&mutex);
	sem_signal(&mutex, 1);

	int pid = fork();
	if (!pid) {
		// Son

		int pid = fork();
		if (!pid) {
			// Grandson
			mi_id = getpid();
			sem_wait(&mutex, 1);

			cpuid = open("/proc/cpuid", FS_OPEN_RDWR);
			*sz = read(cpuid, shared_page, PAGE_SIZE);
			fprintf(con, "\\c0A[%d] Grandson: \\c0F%s", mi_id, shared_page);
			close(cpuid);

			sprintf(private_page, "wefgdcccdla\n");
			fprintf(con, "\\c0A[%d] Grandson: \\c0Fescribo en una pagina privada.\n", mi_id);

			cpuid = open("/proc/cpuid", FS_OPEN_RDWR);
			*sz = read(cpuid, shared_page, PAGE_SIZE);
			fprintf(con, "\\c0A[%d] Grandson: \\c0F%s\n", mi_id, shared_page);
			close(cpuid);

			sem_signal(&mutex, 1);

			cleanup();
			return res;
		}

		mi_id = getpid();
		sem_wait(&mutex, 1);

		cpuid = open("/proc/cpuid", FS_OPEN_RDWR);
		*sz = read(cpuid, shared_page, PAGE_SIZE);
		fprintf(con, "\\c09[%d] Son: \\c0F%s", mi_id, shared_page);
		close(cpuid);

		sprintf(private_page, "hola manola\n");
		fprintf(con, "\\c09[%d] Son: \\c0Fescribo en una pagina privada.\n", mi_id);

		cpuid = open("/proc/cpuid", FS_OPEN_RDWR);
		*sz = read(cpuid, shared_page, PAGE_SIZE);
		fprintf(con, "\\c09[%d] Son: \\c0F%s\n", mi_id, shared_page);
		close(cpuid);

		sem_signal(&mutex, 1);

		cleanup();
		return res;
	}

	// Grandfather

	mi_id = getpid();
	sem_wait(&mutex, 1);

	cpuid = open("/proc/cpuid", FS_OPEN_RDWR);
	*sz = read(cpuid, shared_page, PAGE_SIZE);
	fprintf(con, "\\c0D[%d] Grandfather: \\c0F%s", mi_id, shared_page);
	close(cpuid);

	sprintf(private_page, "hweggwa\n");
	fprintf(con, "\\c0D[%d] Grandfather: \\c0Fescribo en una pagina privada.\n", mi_id);

	cpuid = open("/proc/cpuid", FS_OPEN_RDWR);
	*sz = read(cpuid, shared_page, PAGE_SIZE);
	fprintf(con, "\\c0D[%d] Grandfather: \\c0F%s\n", mi_id, shared_page);
	close(cpuid);

	sem_signal(&mutex, 1);

	getch(con);
	cleanup();
	return res;
}

static void cleanup() {
	close(con);
	sem_close(&mutex);
}

static int init_shared_page(void** page) {
	*page = palloc();
	if (!*page) {
		fprintf(con, "Failure to allocate page\n");
		return -2;
	}

	if (share_page(*page)) {
		fprintf(con, "Failure to share page\n");
		return -3;
	}

	memset(*page, 0, PAGE_SIZE);
	return 0;
}
