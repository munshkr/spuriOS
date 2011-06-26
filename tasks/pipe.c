#include <syscalls.h>
#include <i386.h>
#include <fs.h>
#include <lib.h>

char buf[4096] = "";

int main () {
	int con = open("/console", FS_OPEN_RDWR);

	sem_t sema;
	sem_open(&sema);

	int pipe_fds[2];
	int res = pipe(pipe_fds);

	if (res) {
		fprintf(con, "Pipe failed! %d\n", res);
	} else {
		fprintf(con, "Pipe %u for reading, %u for writing\n", pipe_fds[0], pipe_fds[1]);
		int pid = fork();

		if (pid != 0) {
			int my_pid = getpid();

			fprintf(con, "[%d] Close read fd\n", my_pid);
			close(pipe_fds[0]);
	
			fprintf(con, "[%d] Write 'Hola mundo!'\n", my_pid);
			fprintf(pipe_fds[1], "Hola mundo!");

			fprintf(con, "[%d] Write ' more text...'\n", my_pid);
			fprintf(pipe_fds[1], " more text...");

			fprintf(con, "[%d] Wait for child %d to finish reading\n", my_pid, pid);
			sem_wait(&sema, 1);

			fprintf(con, "[%d] Try to write and fail (broken pipe): ", my_pid);
			int r = write(pipe_fds[1], "x", 1);
			fprintf(con, "bytes written = %d\n", r);

			fprintf(con, "[%d] Close write fd\n", my_pid, pid);
			close(pipe_fds[1]);

			fprintf(con, "[%d] Return\n", my_pid);
		} else {
			// Child
			int my_pid = getpid();

			fprintf(con, "[%d] Close write fd\n", my_pid);
			close(pipe_fds[1]);

			fprintf(con, "[%d] Read 5 bytes: ", my_pid);
			read(pipe_fds[0], buf, 5);
			fprintf(con, "'%s'\n", buf);

			fprintf(con, "[%d] Read another 5 bytes: ", my_pid);
			read(pipe_fds[0], buf, 5);
			fprintf(con, "'%s'\n", buf);

			fprintf(con, "[%d] Read remaining 14 bytes: ", my_pid);
			read(pipe_fds[0], buf, 14);
			fprintf(con, "'%s'\n", buf);

			fprintf(con, "[%d] Close read fd\n", my_pid);
			close(pipe_fds[0]);

			fprintf(con, "[%d] Signal parent\n", my_pid);
			sem_signal(&sema, 1);

			fprintf(con, "[%d] Return\n", my_pid);
			return 0;
		}
	}

	while (1) { sleep(10); };
	
	close(con);
	return 0;
}
