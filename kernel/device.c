#include <device.h>
#include <errors.h>
#include <proc.h>
#include <con.h>
#include <debug.h>

extern pid cur_pid;

fd_t device_descriptor(chardev* dev) {
	kassert(dev != NULL);

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
			devices[pid][fd] = NULL;
		}
	}
}

inline void init_dev_modules() {
	proc_init();
	con_init();
}

void device_init(void) {
	debug_log("initializing device drivers");
	init_fd_association();
	init_dev_modules();		
}

/* Syscalls */
int close(int fd) {
	if (fd < 0 || fd > MAX_FD) {
		return -EINVALID;
	}

	device* dev = devices[cur_pid][fd];
	if (dev == NULL) {
		return -EINVALID;
	}

	kassert(dev->refcount > 0);
	dev->refcount--;

	if (dev->refcount == 0) {
		devices[cur_pid][fd] = NULL;
		if (dev->flush != NULL) {
			return dev->flush(dev);
		}
	}

	return 0;
}

int read(int fd, void* buf, uint_32 size) {
	if (fd < 0 || fd > MAX_FD) {
		return -EINVALID;
	}

	chardev* dev = (chardev*) devices[cur_pid][fd];
	if (dev == NULL) {
		return -EINVALID;
	}

	if (dev->read) {
		return dev->read(dev, buf, size);
	}

	return -ENOFUNC;
}

int write(int fd, const void* buf, uint_32 size) {
	if (fd < 0 || fd > MAX_FD) {
		return -EINVALID;
	}

	chardev* dev = (chardev*) devices[cur_pid][fd];
	if (dev == NULL) {
		return -EINVALID;
	}

	if (dev->write) {
		return dev->write(dev, buf, size);
	}

	return -ENOFUNC;
}

int seek(int fd, uint_32 size) {
	if (fd < 0 || fd > MAX_FD) {
		return -EINVALID;
	}

	chardev* dev = (chardev*) devices[cur_pid][fd];
	if (dev == NULL) {
		return -EINVALID;
	}

	if (dev->seek) {
		return dev->seek(dev, size);
	}

	return -ENOFUNC;
}
