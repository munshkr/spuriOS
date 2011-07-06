#ifndef __SERIAL_H__
#define __SERIAL_H__

#include <tipos.h>
#include <device.h>

#ifdef __KERNEL__

#define SERIAL_BUFFER_LENGTH 50

struct str_dev_serial {
	uint_32 klass;
	uint_32 refcount;
	chardev_flush_t* flush;
	chardev_read_t* read;
	chardev_write_t* write;
	chardev_seek_t* seek;

	uint_32 io_port;
	pid read_queue;

	bool buffer_free;
	char buffer[SERIAL_BUFFER_LENGTH];
	uint_8 ptr_to;

	pid write_queue;
} __attribute__((packed));

typedef struct str_dev_serial dev_serial;

#define SERIAL_PORTS 2
dev_serial serial_devs[SERIAL_PORTS];

void serial_init();

chardev* serial_open(sint_32 nro, uint_32 flags);

#endif

#endif
