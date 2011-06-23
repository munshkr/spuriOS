#include <device.h>
#include <errors.h>
#include <proc.h>
#include <con.h>
#include <debug.h>
#include <serial.h>
#include <hdd.h>
#include <fs.h>
#include <pipe.h>

extern pid cur_pid;

void device_copy_fds(pid source, pid dest) {
	fd_t fd = 0;
	for (fd = 0; fd < MAX_FD; fd++) {
		devices[dest][fd] = devices[source][fd];
		if (devices[dest][fd]) {
			devices[dest][fd]->refcount++;
		}
	}
}

void device_close_fds_for(pid id) {
	fd_t fd = 0;
	for (fd = 0; fd < MAX_FD; fd++) {
		if (devices[id][fd]) {
			close(fd);
		}
	}
}

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
	con_init("/disk/bin/screen_saver.pso");
	serial_init();
	hdd_init();
	fs_init();
	pipe_init();
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

