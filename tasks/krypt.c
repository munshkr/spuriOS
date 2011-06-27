#include <syscalls.h>
#include <i386.h>
#include <fs.h>
#include <lib.h>

#define PAGE_SIZE 4096

static int con;
static int pipe_read2enc[2];
static int pipe_enc2write[2];

static int krypt_read() {
	fprintf(con, "[read] Open file /disk/kernel.bin as read-only\n");
	int file = open("/disk/kernel.bin", FS_OPEN_RD);
	if (file < 0) {
		fprintf(con, "[read] Failure to open file\n");
		return -1;
	}

	fprintf(con, "[read] Allocating a page for buffering\n");
	char* buffer = palloc();
	if (!buffer) {
		fprintf(con, "[read] Failure to allocate page\n");
		return -2;
	}

	bool eof = FALSE;
	int i, sz;
	while (!eof) {
		memset(buffer, 0, PAGE_SIZE);

		// Because of a limitation in our HDD driver, we can read by sectors at once.
		// With this for loop, we fill the 4Kb buffer and then send it through the pipe.
		for (i = 0; i < PAGE_SIZE / 512; i++) {
			fprintf(con, "[read] Read into (buffer + %d): ", 512 * i);
			sz = read(file, buffer + (512*i), 512);
			fprintf(con, "%d\n", sz);
			if (!sz) {
				eof = TRUE;
				break;
			}
		}

		// Write to pipe shared with 'encrypt' process
		fprintf(con, "[read] Write buffer through `pipe_read2enc`\n");
		write(pipe_read2enc[1], buffer, PAGE_SIZE);
	}

	close(file);
	return 0;
}

static int krypt_encrypt() {
	while (1) { sleep(10); };
	//getch(con);

	return 0;
}

static int krypt_write() {
	while (1) { sleep(10); };
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

		int pid = fork();
		if (!pid) {
			close(pipe_read2enc[0]);
			close(pipe_enc2write[1]);

			res = krypt_write();

			close(pipe_enc2write[0]);
			close(con);
			return res;
		}

		close(pipe_enc2write[0]);

		res = krypt_encrypt();

		close(pipe_enc2write[1]);
		close(pipe_read2enc[0]);
		close(con);
		return res;
	}

	close(pipe_read2enc[0]);

	res = krypt_read();

	close(pipe_read2enc[1]);
	close(con);
	return res;
}
