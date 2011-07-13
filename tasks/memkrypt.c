#include <syscalls.h>
#include <i386.h>
#include <fs.h>
#include <lib.h>

#define PAGE_SIZE 4096
#define BUFFER_PAGES 4    // max 1024 pages

const char* FILENAME = "/disk/kernel.bin";

const char* CIPHER_KEY = "SpuriOS";
const size_t CIPHER_KEY_SIZE = 7;

static int con = 0;
static char* buffer[BUFFER_PAGES] = {};
static sem_t sem_read, sem_encrypt, sem_write;

// Store last size read from HDD file in a particular page (including 0 from
// EOF). `last_size_per_page` points to a shared page. `mutex` must be used when
// reading/writing `last_size_per_page`.
static int* last_size_per_page = NULL;
static sem_t mutex;

static void set_last_size(int i, int sz);
static int get_last_size(int i);

static void cleanup();
static int init_shared_page(void** page);


static int memkrypt_read() {
	fprintf(con, "[read] Hi!\n");

	fprintf(con, "[read] Open %s as read-only\n", FILENAME);
	int file = open(FILENAME, FS_OPEN_RD);
	if (file < 0) {
		fprintf(con, "[read] Failure to open file\n");
		return -1;
	}

	// Mark all pages as available
	sem_signal(&sem_read, BUFFER_PAGES);

	int i = 0;
	while (TRUE) {
		sem_wait(&sem_read, 1);

		memset(buffer[i], 0, PAGE_SIZE);

		fprintf(con, "[read] Read into buffer\n");
		int sz = read(file, buffer[i], PAGE_SIZE);

		set_last_size(i, sz);

		sem_signal(&sem_encrypt, 1);

		if (!sz) break;
		i = (i + 1) % BUFFER_PAGES;
	}

	fprintf(con, "[read] Close file\n");
	close(file);

	fprintf(con, "[read] Done\n");
	return 0;
}

static int memkrypt_encrypt() {
	fprintf(con, "[encrypt] Hi!\n");

	int i = 0;
	int bytes = 0;
	while (TRUE) {
		sem_wait(&sem_encrypt, 1);

		int sz = get_last_size(i);

		fprintf(con, "[encrypt] Encrypt buffer with cipher key\n");
		int j;
		for (j = 0; j < sz; j++, bytes++) {
			buffer[i][j] ^= CIPHER_KEY[bytes % CIPHER_KEY_SIZE];
		}

		sem_signal(&sem_write, 1);

		if (!sz) break;
		i = (i + 1) % BUFFER_PAGES;
	}

	fprintf(con, "[encrypt] Done\n");
	return 0;
}

static int memkrypt_write() {
	fprintf(con, "[write] Hi!\n");

	fprintf(con, "[write] Open /serial0 as write-only\n");
	int file = open("/serial0", FS_OPEN_WR);
	if (file < 0) {
		fprintf(con, "[write] Failure to open file\n");
		return -1;
	}

	int i = 0;
	while (TRUE) {
		sem_wait(&sem_write, 1);

		int sz = get_last_size(i);
		fprintf(con, "[write] last_size(%d) = %d\n", i, sz);

		fprintf(con, "[write] Write %db to serial port\n", sz);
		int wsz = write(file, buffer[i], sz);
		fprintf(con, "[write] Wrote %db\n", wsz);

		sem_signal(&sem_read, 1);

		if (!sz) break;
		i = (i + 1) % BUFFER_PAGES;
	}

	fprintf(con, "[write] Close file\n");
	close(file);

	fprintf(con, "[write] Done\n");
	return 0;
}


int main () {
	con = open("/console", FS_OPEN_RDWR);
	if (con < 0) {
		return -1;
	}

	fprintf(con, "Allocate and share pages for buffering\n");
	init_shared_page((void**) &last_size_per_page);

	int res = 0, i;
	for (i = 0; i < BUFFER_PAGES; i++) {
		// TODO check result
		init_shared_page((void**) &buffer[i]);
		last_size_per_page[i] = 0;
	}

	sem_open(&sem_read);
	sem_open(&sem_encrypt);
	sem_open(&sem_write);

	sem_open(&mutex);
	sem_signal(&mutex, 1);

	int pid = fork();
	if (!pid) {

		int pid = fork();
		if (!pid) {
			res = memkrypt_write();

			getch(con);
			cleanup();
			return res;
		}

		res = memkrypt_encrypt();

		cleanup();
		return res;
	}

	res = memkrypt_read();

	cleanup();
	return res;
}

static void cleanup() {
	close(con);
	sem_close(&sem_read);
	sem_close(&sem_encrypt);
	sem_close(&sem_write);
	sem_close(&mutex);
}

static void set_last_size(int i, int sz) {
	sem_wait(&mutex, 1);
	last_size_per_page[i] = sz;
	sem_signal(&mutex, 1);
}

static int get_last_size(int i) {
	sem_wait(&mutex, 1);
	int sz = last_size_per_page[i];
	sem_signal(&mutex, 1);
	return sz;
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
