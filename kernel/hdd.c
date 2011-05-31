#include <hdd.h>
#include <i386.h>
#include <loader.h>
#include <pic.h>
#include <idt.h>
#include <common.h>
#include <vga.h>
#include <debug.h>

#define C(x) ((dev_hdd*) x)

#define PRI_IRQ 14
#define SEC_IRQ 15

#define PRI_PORT 0x1F0
#define SEC_PORT 0x170

#define PRI_CTRL_PORT 0x3F0
#define SEC_CTRL_PORT 0x370

#define P_DATA			0
#define P_FEATURE		1
#define P_SECTOR_COUNT	2
#define P_LBA_LO		3
#define P_LBA_MID		4
#define P_LBA_HI		5
#define P_DRIVE			6
#define P_CMD_ST		7

#define P_CTRL_NIEN		6

#define CMD_READ		0x20
#define CMD_WRITE		0x30

#define ST_ERR			1
#define ST_DRQ			8
#define ST_SRV			16
#define ST_DF			32
#define ST_RDY			64
#define ST_BSY			128

#define DRIVE_MASTER	0xE0
#define DRIVE_SLAVE		0xF0

sint_32 hdd_block_write(blockdev* self, uint_32 pos, const void* buf, uint_32 size) {
	return -1; //Opcional
}

sint_32 hdd_block_read(blockdev* self, uint_32 pos, void* buf, uint_32 size) {
	if (C(self)->size != size) {
		return -1;
	} else {
		if (C(self)->read_queue != FREE_QUEUE) {
			return -1;
		}

		uint_32 port = C(self)->port;
		uint_32 ctrl_port = C(self)->ctrl_port;

		uint_8 status = 0;

		// XXX NECCESARY (?) 400ms delay (we read four times de status port)
		uint_32 times;
		for (times = 0; times < 5; times++) {
			status = inb(ctrl_port + P_CTRL_NIEN);
		}
		
		outb(port + P_DRIVE, C(self)->channel | ((pos >> 24) & 0xF));
		outb(port + P_SECTOR_COUNT, 1);
		outb(port + P_LBA_LO, (uint_8) pos);
		outb(port + P_LBA_MID, (uint_8) (pos >> 8));
		outb(port + P_LBA_HI, (uint_8) (pos >> 16));
		outb(port + P_CMD_ST, CMD_READ);

		loader_enqueue(&(C(self)->read_queue));

		// READ
		breakpoint();

		status = inb(port + P_CMD_ST);
		if (status & ST_ERR) {
			debug_log("HDD: ERR bit set while reading");
			return -1;
		} else if (status & ST_DF) {
			debug_log("HDD: Drive Fault while reading");
			return -1;
		} else if ((status & ST_BSY) || !(status & ST_RDY)) {
			debug_log("HDD: Device busy while reading");
			return -1;
		} else if (!(status & ST_DRQ)) {
			debug_log("HDD: No data to transfer");
			return -1;
		}

		uint_32 i;
		uint_32 words_to_read = self->size / 2;
		for (i = 0; i < words_to_read; i++) {
			uint_16 data = inw(port + P_DATA);
			*((uint_16*) buf) = data;
			buf += 2;
		}

		return self->size;
	}
}

blockdev* hdd_open(int hdd) {
	if (hdd < 0 || hdd >= HARD_DISK_DRIVES) {
		return 0;
	} else {
		hdd_devs[hdd].refcount++;
		return (blockdev*)&hdd_devs[hdd];
	}
}

dev_hdd* device_for(uint_32 port, bool isSlave) {
	if (port == PRI_PORT) {
		return &(hdd_devs[(isSlave ? HDD_PRI_SLAVE : HDD_PRI_MASTER)]);
	} else if (port == SEC_PORT) {
		return &(hdd_devs[(isSlave ? HDD_SEC_SLAVE : HDD_SEC_MASTER)]);
	} else {
		kassert("device not found");
		return 0;
	}
}

void hdd_primary_handler(registers_t* regs) {
	uint_8 drive = inb(PRI_PORT + P_DRIVE);
	dev_hdd* dev = device_for(PRI_PORT, drive & 1);
	loader_unqueue(&(dev->read_queue));
}

void init_hdd(uint_32 hdd, uint_32 port, uint_32 ctrl_port, uint_8 channel) {
	hdd_devs[hdd].klass = CLASS_DEV_HDD;
	hdd_devs[hdd].refcount = 0;
	hdd_devs[hdd].flush = 0;
	hdd_devs[hdd].read = hdd_block_read;
	hdd_devs[hdd].write = 0;
	hdd_devs[hdd].size = 512;
	hdd_devs[hdd].read_queue = FREE_QUEUE;
	hdd_devs[hdd].port = port;
	hdd_devs[hdd].ctrl_port = ctrl_port;
	hdd_devs[hdd].channel = channel;
}

void hdd_init() {
	idt_register(ISR_IRQ14, hdd_primary_handler, PL_KERNEL);
	pic_clear_irq_mask(2); // Cascade for Slave PIC
	pic_clear_irq_mask(PRI_IRQ);

//  XXX Disable by now (till we make a good IRQ15 handler)
//	pic_clear_irq_mask(SEC_IRQ);

	init_hdd(HDD_PRI_MASTER, PRI_PORT, PRI_CTRL_PORT, DRIVE_MASTER);
	init_hdd(HDD_PRI_SLAVE, PRI_PORT, PRI_CTRL_PORT, DRIVE_SLAVE);
	init_hdd(HDD_SEC_MASTER, SEC_PORT, SEC_CTRL_PORT, DRIVE_MASTER);
	init_hdd(HDD_SEC_SLAVE, SEC_PORT, SEC_CTRL_PORT, DRIVE_SLAVE);
}


