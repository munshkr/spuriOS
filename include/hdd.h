#ifndef __HDD_H__
#define __HDD_H__


#ifdef __KERNEL__
#include <tipos.h>
#include <device.h>

struct str_dev_hdd {
	uint_32 klass;
	uint_32 refcount;
	blockdev_flush_t* flush;
	blockdev_read_t* read;
	blockdev_write_t* write;
	uint_32 size;
	// Proper attributes
	pid read_queue;
	uint_32 port;
	uint_32 ctrl_port;
	uint_8 channel;
} __attribute__((packed));

typedef struct str_dev_hdd dev_hdd;

#define HARD_DISK_DRIVES	4
#define HDD_PRI_MASTER		0
#define HDD_PRI_SLAVE		1
#define HDD_SEC_MASTER		2
#define HDD_SEC_SLAVE		3
dev_hdd hdd_devs[HARD_DISK_DRIVES];

blockdev* hdd_open(int nro);
sint_32 hdd_block_read(blockdev* self, uint_32 pos, void* buf, uint_32 size);

void hdd_init();

#endif

#endif

