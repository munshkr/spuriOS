#include <syscalls.h>
#include <loader.h>
#include <lib.h>
#include <fs.h>
#include <i386.h>

#define EXEC_CHAR ':'
#define CAT_CHAR '@'
#define EXIT_CMD "exit"
#define HELP_CMD "help"

#define SPURSH_EXIT 1

const size_t LINE_BUFFER_SIZE = 80;

static void print_prompt(void);
static int parse(char* line_buffer, uint_32 size);

int con_fd;

int main() {
	char line_buffer[LINE_BUFFER_SIZE];
	size_t size;
	bool eof;

	con_fd = open("/console", FS_OPEN_RDWR);
	fprintf(con_fd, "Spursh v0.1\n");
	fprintf(con_fd, "Type 'help' when you're lost\n\n");

	int parse_code = 0;
	while (!parse_code) {
		parse_code = 0;

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
			parse_code = parse(line_buffer, size);
		}
	}

	return 0;
}

static void print_prompt(void) {
	fprintf(con_fd, ">> ");
}

static void cmd_execute(char* file, uint_32 size) {
	if (!size) {
		fprintf(con_fd, "Spursh: Expected filename after ':'\n");
	} else {
		sint_32 throwed_pid = run(file);
		if (throwed_pid > 0) {
			fprintf(con_fd, "Spursh: pid [%d]\n", throwed_pid);
		} else if (throwed_pid == -RUN_ERROR_OPENING) {
			fprintf(con_fd, "Spursh: Error opening %s\n", file);
		} else if (throwed_pid == -RUN_INVALID_EXECUTABLE) {
			fprintf(con_fd, "Spursh: %s is not a valid PSO file\n", file);
		} else if (throwed_pid == -RUN_ERROR_READING) {
			fprintf(con_fd, "Spursh: Error reading %s\n", file);
		} else if (throwed_pid == -RUN_UNAVAILABLE_MEMORY) {
			fprintf(con_fd, "Spursh: Not enough memory\n", file);
		}
	}
}

static void cmd_cat(char* file, uint_32 size) {
	if (!size) {
		fprintf(con_fd, "Spursh: Expected filename after '@'\n");
		return;
	}

	int fd = open(file, FS_OPEN_RD);
	if (fd < 0) {
		fprintf(con_fd, "Spursh: Error opening %s\n", file);
		return;
	}

	char buf[513];

	while (1) {
		int readed = read(fd, &buf, 512);
		if (readed < 1) {
			close(fd);
			return;
		} else {
			buf[readed] = 0;
			fprintf(con_fd, "%s", &buf);
		}
	}
}

static int parse(char* line_buffer, uint_32 size) {
	if (*line_buffer == EXEC_CHAR) {
		cmd_execute(&line_buffer[1], size - 1);
	} else if (*line_buffer == CAT_CHAR) {
		cmd_cat(&line_buffer[1], size - 1);
	} else if (!strcmp(line_buffer, EXIT_CMD)) {
		fprintf(con_fd, "Bye.\n");
		close(con_fd);
		return -SPURSH_EXIT;
	} else if (!strcmp(line_buffer, HELP_CMD)) {
		fprintf(con_fd, "Available commands:\n");
		fprintf(con_fd, "\thelp\tShow this message\n");
		fprintf(con_fd, "\texit\tTerminate this console\n");
		fprintf(con_fd, "\t:PATH\tRun a .PSO task at PATH\n");
		fprintf(con_fd, "\t@PATH\tRead and print at PATH\n");
		fprintf(con_fd, "\n");
	} else {
		fprintf(con_fd, "Spursh: Huh? What does \"");
		write(con_fd, line_buffer, size);
		fprintf(con_fd, "\" mean?\n");
	}

	return 0;
}
