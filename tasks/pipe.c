#include <syscalls.h>
#include <i386.h>
#include <fs.h>
#include <lib.h>

char buf[4096] = "";

int main () {
	int con = open("/console", FS_OPEN_RDWR);

	sint_32 pipe_fds[2];
	sint_32 res = pipe(pipe_fds);
	if (res) {
		fprintf(con, "Pipe failed! %d\n", res);
	} else {
		fprintf(con, "Pipe %u for reading, %u for writing\n", pipe_fds[0], pipe_fds[1]);
		int pid = fork();

		if (pid != 0) {
			close(pipe_fds[0]);
	
			fprintf(con, "- Write 'Hola mundo!'\n");
			write(pipe_fds[1], "Hola mundo!", 11);

			fprintf(con, "- Write ' more text...'\n");
			fprintf(pipe_fds[1], " more text...");

			fprintf(con, "done\n");

			close(pipe_fds[1]);
		} else {
			// Child
			close(pipe_fds[1]);

			fprintf(con, "- Read 5 bytes: ");
			read(pipe_fds[0], buf, 5);
			fprintf(con, "'%s'\n", buf);

			fprintf(con, "- Read another 5 bytes: ");
			read(pipe_fds[0], buf, 5);
			fprintf(con, "'%s'\n", buf);

			fprintf(con, "- Read remaining 14 bytes: ");
			read(pipe_fds[0], buf, 14);
			fprintf(con, "'%s'\n", buf);

			close(pipe_fds[0]);
		}
	}

	while (1) { sleep(10); };
	
	close(con);
	return 0;
}
