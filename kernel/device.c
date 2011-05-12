#include <device.h>
#include <errors.h>

extern pid cur_pid;

int device_descriptor(chardev* dev) {
	uint_32 fd;
	for (fd = 0; fd < MAX_FD; fd++) {
		if (!devices[cur_pid][fd]) {
			devices[cur_pid][fd] = (device*) dev;
			dev->refcount++;
			break;
		}
	}

	return fd;
}

void device_init(void) {
	uint_32 pid, fd;
	for (pid = 0; pid < MAX_PID; pid++) {
		for (fd = 0; fd < MAX_FD; fd++) {
			devices[pid][fd] = 0;
		}
	}
}
