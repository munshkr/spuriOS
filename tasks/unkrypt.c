#include <syscalls.h>
#include <i386.h>
#include <fs.h>
#include <lib.h>

#define PAGE_SIZE 4096

const char* CIPHER_KEY = "SpuriOS";
const size_t CIPHER_KEY_SIZE = 7;


int main() {
	int con = open("/console", FS_OPEN_RDWR);
	if (con < 0) {
		return -1;
	}

	char* buffer = palloc();
	if (!buffer) {
		fprintf(con, "Failed to allocate page\n");
		getch(con);
		close(con);
		return -2;
	}

	int serial = open("/serial0", FS_OPEN_RD);
	if (serial < 0) {
		fprintf(con, "Failed to open serial port, error %d\n", serial);
		getch(con);
		close(con);
		return -3;
	}

	int bytes = 0;
	while (TRUE) {
		memset(buffer, 0, PAGE_SIZE);

		int sz = read(serial, buffer, PAGE_SIZE);
		if (!sz) break;

		int i;
		for (i = 0; i < sz; i++, bytes++) {
			buffer[i] ^= CIPHER_KEY[bytes % CIPHER_KEY_SIZE];
		}

		write(con, buffer, sz);
	}

	fprintf(con, "Total: %d\n", bytes);

	close(serial);

	fprintf(con, "\nPress any key to finish\n");
	getch(con);
	close(con);

	return 0;
}
