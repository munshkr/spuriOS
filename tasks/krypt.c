#include <syscalls.h>
#include <i386.h>
#include <fs.h>
#include <lib.h>

#define PAGE_SIZE 4096
#define SECTOR_SIZE 512

const char* CIPHER_KEY = "SpuriOS";
const size_t CIPHER_KEY_SIZE = 7;

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

	int i, sz;
	while (TRUE) {
		memset(buffer, 0, PAGE_SIZE);
		sz = 0;

		fprintf(con, "[read] Read into buffer\n");

		// Because of a limitation in our HDD driver, we can read by sectors at once.
		// With this for loop, we fill the 4Kb buffer and then send it through the pipe.
		for (i = 0; i < PAGE_SIZE / SECTOR_SIZE; i++) {
			sz += read(file, buffer + (SECTOR_SIZE * i), SECTOR_SIZE);
			if (!sz) break;
		}

		if (!sz) break;

		// Write to pipe shared with 'encrypt' process
		fprintf(con, "[read] Write %db to `pipe_read2enc`\n", sz);
		write(pipe_read2enc[1], buffer, sz);
	}

	fprintf(con, "[read] Done\n");

	close(file);
	return 0;
}

static int krypt_encrypt() {
	fprintf(con, "[encrypt] Allocating a page for buffering\n");
	char* buffer = palloc();
	if (!buffer) {
		fprintf(con, "[encrypt] Failure to allocate page\n");
		return -2;
	}

	int i, sz;
	while (TRUE) {
		memset(buffer, 0, PAGE_SIZE);

		fprintf(con, "[encrypt] Read into buffer\n");
		sz = read(pipe_read2enc[0], buffer, PAGE_SIZE);
		if (!sz) break;

		fprintf(con, "[encrypt] Encrypt buffer with cipher key\n");
		for (i = 0; i < sz; i++) {
			buffer[i] ^= CIPHER_KEY[i % CIPHER_KEY_SIZE];
		}

		// Write to pipe shared with 'write' process
		fprintf(con, "[encrypt] Write %db to `pipe_enc2write`\n", sz);
		write(pipe_enc2write[1], buffer, sz);
	}

	fprintf(con, "[encrypt] Done\n");

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
