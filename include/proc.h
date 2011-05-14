#ifndef __PROC_H__
#define __PROC_H__

#include <device.h>

struct str_dev_proc_cpuid {
	uint_32 klass;
	uint_32 refcount;
	chardev_flush_t* flush;
	chardev_read_t* read;
	chardev_write_t* write;
	chardev_seek_t* seek;

	// Proper attributes
	uint_32 stream_position;
} __attribute__((packed));

typedef struct str_dev_proc_cpuid dev_proc_cpuid;

// XXX We arbitrary determine max open devs as MAX_FD
#define MAX_PROC_CPUID_DEVS MAX_FD
dev_proc_cpuid proc_cpuid_devs[MAX_PROC_CPUID_DEVS];

chardev* proc_cpuid_open();
void proc_init();

#endif // __PROC_H__
