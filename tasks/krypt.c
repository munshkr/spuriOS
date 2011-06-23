#include <syscalls.h>
#include <i386.h>
#include <fs.h>
#include <lib.h>

static int con;
static int pipe_read2enc[2];
static int pipe_enc2write[2];

static int krypt_read(int pid) {
	fprintf(con, "[read] I'm pid %d\n", pid);
	//while (1) { sleep(10); };
	//getch(con);

	return 0;
}

static int krypt_encrypt(int pid) {
	fprintf(con, "[encrypt] I'm pid %d\n", pid);
	//while (1) { sleep(10); };
	//getch(con);

	return 0;
}

static int krypt_write(int pid) {
	fprintf(con, "[write] I'm pid %d\n", pid);
	//while (1) { sleep(10); };
	//getch(con);

	return 0;
}

int main () {
	con = open("/console", FS_OPEN_RDWR);
	if (con < 0) {
		return -1;
	}

	int res = pipe(pipe_read2enc);
	if (res) {
		fprintf(con, "Failed to open `pipe_read2enc`, error %d\n", res);
		close(con);
		return -2;
	}

	int pid = fork();
	if (!pid) {
		close(pipe_read2enc[1]);

		res = pipe(pipe_enc2write);
		if (res) {
			fprintf(con, "Failed to open `pipe_enc2write`, error %d\n", res);
			close(con);
			return -3;
		}

		pid = fork();
		if (!pid) {
			close(pipe_read2enc[0]);
			close(pipe_enc2write[1]);

			res = krypt_write(getpid());

			close(pipe_enc2write[0]);
			close(con);
			return res;
		}

		close(pipe_enc2write[0]);

		res = krypt_encrypt(getpid());

		close(pipe_enc2write[1]);
		close(pipe_read2enc[0]);
		close(con);
		return res;
	}

	close(pipe_read2enc[0]);

	res = krypt_read(getpid());

	close(pipe_read2enc[1]);
	close(con);
	return res;
}
