#include <syscalls.h>
#include <lib.h>
#include <fs.h>

const size_t LINE_BUFFER_SIZE = 80;

static void print_prompt(void);
static void parse(char* line_buffer, uint_32 size);
static void memset(void* addr, int value, size_t size);

int con_fd;

int main() {
	char line_buffer[LINE_BUFFER_SIZE];
	size_t size;
	bool eof;

	con_fd = open("/console", FS_OPEN_RDWR);
	fprintf(con_fd, "Spursh v0.1\n\n");

	while (1) {
		eof = FALSE;
		memset(line_buffer, 0, LINE_BUFFER_SIZE);
		size = 0;
		print_prompt();

		while (!eof) {
			char sc = 0;
			size_t sz = read(con_fd, &sc, 1);

			if (!sz) continue;

			switch (sc) {
			case '\n':
				eof = TRUE;
				write(con_fd, &sc, 1);
				break;
			case '\b':
				if (size > 0) {
					line_buffer[size] = 0;
					size--;
					write(con_fd, &sc, 1);
				}
				break;
			case '\t':
				break;
			case '\e':
				memset(line_buffer, '\b', size);
				write(con_fd, line_buffer, size);
				memset(line_buffer, 0, LINE_BUFFER_SIZE);
				size = 0;
				break;
			default:
				if (size < LINE_BUFFER_SIZE) {
					line_buffer[size] = sc;
					size++;
					write(con_fd, &sc, 1);
				}
				break;
			}
		}

		if (size > 0) {
			parse(line_buffer, size);
		}
	}

	close(con_fd);

	return 0;
}

static void print_prompt(void) {
	fprintf(con_fd, ">> ");
}

static void parse(char* line_buffer, uint_32 size) {
	// FIXME Use fprintf %s specifier, once bug is fixed
	fprintf(con_fd, "Huh? What does \"");
	write(con_fd, line_buffer, size);
	fprintf(con_fd, "\" mean?\n");
}

// FIXME Delete this and use memset from lib.c
static void memset(void* addr, int value, size_t size) {
	int i;
	for (i = 0; i < size; i++) {
		*(uint_8*)addr = value;
		addr++;
	}
}
