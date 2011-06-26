#include <syscalls.h>
#include <i386.h>
#include <fs.h>
#include <lib.h>

sem_t sem_p, sem_c;
int con;

static void cleanup() {
	sem_close(&sem_c);
	sem_close(&sem_p);
	close(con);
}

int main () {
	con = open("/console", FS_OPEN_RDWR);

	char* shared = palloc();

	if (share_page(shared)) {
		fprintf(con, "No se pudo compartir la pagina\n");
		return -1;
	}

	memset(shared, 0, 4096);

	sem_open(&sem_p);
	sem_open(&sem_c);

	int pid = fork();

	if (!pid) {
		// Child process
		sem_wait(&sem_c, 1);

		if (strcmp(shared, "Hola hijo...\n")) {
			fprintf(con, "Mi padre no me habla!!!");
			getch(con);
			cleanup();
			return -1;
		}

		fprintf(con, "Mi padre me ha hablado!!!\n");

		sprintf(shared, "Hola padre!!!\n");
		sem_signal(&sem_p, 1);
		sem_wait(&sem_c, 1);

		if (strcmp(shared, "Duerme ninio\n")) {
			fprintf(con, "Mi padre me abandona!!!");
			getch(con);
			cleanup();
			return -1;
		}

		fprintf(con, "Mi padre me ha dicho que duerma!!!\n");

		sprintf(shared, "Adios.\n");
		sem_signal(&sem_p, 1);

		cleanup();
		return 0;
	}

	sprintf(shared, "Hola hijo...\n");

	sem_signal(&sem_c, 1);
	sem_wait(&sem_p, 1);

	if (strcmp(shared, "Hola padre!!!\n")) {
		fprintf(con, "Mi hijo no me escucha...\n");
		getch(con);
		cleanup();
		return -1;
	}

	fprintf(con, "Mi hijo me ha escuchado...\n");

	sprintf(shared, "Duerme ninio\n");
	sem_signal(&sem_c, 1);
	sem_wait(&sem_p, 1);

	if (strcmp(shared, "Adios.\n")) {
		fprintf(con, "Mi hijo no me hace caso, changos\n");
		getch(con);
		cleanup();
		return -1;
	}

	fprintf(con, "Press any key to finish\n");
	getch(con);

	cleanup();
	return 0;
}
