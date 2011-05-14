#include <device.h>
#include <errors.h>
#include <proc.h>
#include <debug.h>

extern pid cur_pid;

fd_t device_descriptor(chardev* dev) {
	fd_t fd;
	for (fd = 0; fd < MAX_FD; fd++) {
		if (!devices[cur_pid][fd]) {
			devices[cur_pid][fd] = (device*) dev;
			dev->refcount++;
			break;
		}
	}

	return fd;
}

inline void init_fd_association() {
	uint_32 pid, fd;
	for (pid = 0; pid < MAX_PID; pid++) {
		for (fd = 0; fd < MAX_FD; fd++) {
			devices[pid][fd] = 0;
		}
	}
}

inline void init_dev_modules() {
	proc_init();
}

void device_init(void) {
	debug_log("initializing device drivers");
	init_fd_association();
	init_dev_modules();		
}
