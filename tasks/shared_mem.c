#include <syscalls.h>
#include <i386.h>
#include <fs.h>
#include <lib.h>

char buf[4096] = "";

int main () {
	int con = open("/console", FS_OPEN_RDWR);

	char* shared = palloc();

	if (share_page(shared)) {
		fprintf(con, "No se pudo compartir la pagina\n");
		return -1;
	}

	sem_t sema;
	sem_open(&sema);

	int pid = fork();

	if (pid != 0) {

		sprintf(shared, "Hola hijo...\n");

		sem_signal(&sema, 1);
		sem_wait(&sema, 1);

		if (strcmp(shared, "Hola padre!!!\n")) {
			fprintf(con, "Mi hijo no me escucha...");
			sem_close(&sema);
			close(con);
			return -1;
		}

		fprintf(con, "Mi hijo me ha escuchado...");

		sprintf(shared, "Duerme ninio\n");
		sem_signal(&sema, 2);
		sem_wait(&sema, 1);

		sem_close(&sema);
	} else {
		// Child
		sem_wait(&sema, 1);

		if (strcmp(shared, "Hola hijo...\n")) {
			fprintf(con, "Mi padre no me habla!!!");
			sem_close(&sema);
			close(con);
			return -1;
		}

		fprintf(con, "Mi padre me ha hablado!!!\n");

		sprintf(shared, "Hola padre!!!\n");
		sem_signal(&sema, 1);
		sem_wait(&sema, 2);

		if (strcmp(shared, "Duerme ninio\n")) {
			fprintf(con, "Mi padre me abandona!!!");
			sem_close(&sema);
			close(con);
			return -1;
		}

		fprintf(con, "Mi padre me ha dicho que duerma!!!\n");

		sprintf(shared, "Adios.\n");
		sem_signal(&sema, 1);
		sem_close(&sema);
	}

	while (1) { sleep(10); };

	close(con);
	return 0;
}

