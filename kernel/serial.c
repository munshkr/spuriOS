#include <serial.h>
#include <tipos.h>
#include <i386.h>
#include <idt.h>
#include <isr.h>
#include <pic.h>
#include <debug.h>
#include <common.h>
#include <mm.h>
#include <fs.h>

#define PORT_1 0x3F8
#define PORT_2 0x2F8

/* Serial Controler sub-SP_PORTs */
#define PORT_DATA  0 /* R/W - DATA flow */
#define PORT_IER   1 /* R/W - Interrupt Enable Register */
#define PORT_IIR   2 /* R   - Interrupt Id Register */
#define PORT_FCTRL 2 /*   W - FIFO Control */
#define PORT_LCTRL 3 /* R/W - Line Control */
#define PORT_MCTRL 4 /* R/W - MODEM Control */
#define PORT_LSTAT 5 /* R/W - Line Status */
#define PORT_MSTAT 6 /* R/W - MODEM Status */
#define PORT_SCRAT 7 /* R/W - Scratch ¿eh? */
#define PORT_DL_LSB 0 /* Divisor latch - LSB (need DLAB=1) */
#define PORT_DL_MSB 1 /* Divisor latch - MSB (need DLAB=1)  */

/*** REMEMBER: Don't use drugs while designing a chip:
 *
 * 8.10 SCRATCHPAD REGISTER
 * This 8-bit Read Write Register does not control the UART
 * in anyway It is intended as a scratchpad register to be used
 * by the programmer to hold data temporarily
 */

/* Line Control Bits (type selection, and DLAB) */
#define LC_BIT5 0x00
#define LC_BIT6 0x01
#define LC_BIT7 0x02
#define LC_BIT8 0x03

#define LC_PARITY   0x08
#define LC_PARITY_E 0x10
#define LC_PARITY_O 0x00
#define LC_PARITY_ST 0x20
#define LC_BREAK    0x40
#define LC_DLAB     0x80

/* Line Status Bits */
#define LS_DR   0x01  /* Data Ready */
#define LS_OE   0x02  /* Overrun Error indicator (reading) */
#define LS_PE   0x04  /* Parity Error */
#define LS_FE   0x08  /* Framing Error (invalid stop bit) */
#define LS_BI   0x10  /* Break Interrupt */
#define LS_THRE 0x20  /* Transmitter Holding Register Empty */
#define LS_TEMT 0x40  /* Transmitter Empty */

/* FIFO Control Register Bits */
#define FC_FIFO       0x01 /* Enable FIFO (in y out) */
#define FC_CL_RCVFIFO 0x02 /* Clear RCVR FIFO */
#define FC_CL_XMTFIFO 0x04 /* Clear XMIT FIFO */
#define FC_MODE1      0x08 /* No tengo ni la más puta idea de qué hace este bit */
#define FC_TRIGGER_01 0x00 /* Trigger at 1-byte */
#define FC_TRIGGER_04 0x40 /* Trigger at 4-bytes */
#define FC_TRIGGER_08 0x80 /* Trigger at 8-bytes */
#define FC_TRIGGER_14 0xC0 /* Trigger at 14-bytes */

/* Iterrupt Id Bits */
#define II_nINTPEND  0x01 /* No interrupt pending */
#define II_ID_MASK   0x0E /* Mascara de IDs */
#define II_ID_RLS    0x06 /* Int ID: Receiver Line Status Interrupt */
#define II_ID_RDA    0x04 /* Int ID: Receiver Data Available */
#define II_ID_CTI    0x0C /* Int ID: Character Timeout Indicator */
#define II_ID_THRE   0x02 /* Int ID: Transmitter Holding Register Empty */
#define II_ID_MODEM  0x00 /* Int ID: MODEM Status */
#define II_INT_TOUT  0x08

/* Interrupt Enable Bits */
#define IE_RDA   0x01 /* Int Enable: Receiver Data Available */
#define IE_THRE  0x02 /* Int Enable: Transmitter Holding Register Empty */
#define IE_RLS   0x04 /* Int Enable: Receiver Line Status */
#define IE_MODEM 0x08 /* Int Enable: MODEM Status */

#define C(x) ((dev_serial*) x)

void read_from_serial(uint_32 index) {
	dev_serial* device = &serial_devs[index];
	uint_32 port = device->io_port;
	uint_8 intid = inb(port + PORT_IIR);
	if (intid & II_ID_RDA) {
		char data = inb(port);

		if (device->ptr_to < SERIAL_BUFFER_LENGTH) {
			device->buffer[device->ptr_to] = data;
			device->ptr_to++;
			device->buffer_free = FALSE;
		}

		if (device->read_queue != FREE_QUEUE) {
			loader_unqueue(&(device->read_queue));
		}
	} else if (intid & II_ID_THRE) {
		loader_unqueue(&(device->write_queue));
	}
}

void com_1_3_handler(registers_t* regs) {
	read_from_serial(0); // FIXME 1 or 3
}

void com_2_4_handler(registers_t* regs) {
	read_from_serial(1); // FIXME 2 or 4
}

sint_32 serial_read(chardev* self, void* buf, uint_32 size) {
	if (size > 0) {
		if (C(self)->buffer_free) {
			loader_enqueue(&(C(self)->read_queue));
		}

		uint_32 bytes_copied;
		uint_32 total_copy = 0;

		bytes_copied = (size < C(self)->ptr_to ? size : C(self)->ptr_to);

		copy2user(&(C(self)->buffer), buf, bytes_copied);
		C(self)->ptr_to = 0;
		C(self)->buffer_free = TRUE;

		total_copy = bytes_copied;
		return total_copy;
	} else {
		return size;
	}
}

sint_32 serial_write(chardev* self, const void* buf, uint_32 size) {
	uint_32 buf_pl = mm_pl_of_vaddr((void*) buf, cur_pdt());
	if (buf_pl == PL_USER) {
		uint_32 port = C(self)->io_port;
		uint_32 writed = 0;
		while (writed < size) {
			if (inb(port + PORT_LSTAT) & LS_THRE) {
				outb(port, *(char*)buf);
				buf++;
				writed++;
			}
			loader_sleep(1);
		}
		return writed;
	} else {
		return -1;
	}
}

uint_32 serial_flush(chardev* self) {
	C(self)->klass = CLASS_DEV_NONE;
	C(self)->read = 0;
	C(self)->write = 0;

	return 0;
}

chardev* serial_open(sint_32 index, uint_32 flags) { /* 0 for COM1, 1 for COM2 and so on */
	// FIXME Attributes R/W, pass it
	if (index < 0 || index >= SERIAL_PORTS) {
		return 0;
	} else if (serial_devs[index].klass != CLASS_DEV_NONE) {
		return 0;
	} else {
		if (flags & FS_OPEN_RD) {
			serial_devs[index].read = serial_read;
		}
		if (flags & FS_OPEN_WR) {
			serial_devs[index].write = serial_write;
		}

		serial_devs[index].read_queue = FREE_QUEUE;
		serial_devs[index].write_queue = FREE_QUEUE;
		serial_devs[index].buffer_free = TRUE;
		//serial_devs[index].ptr_from = 0;
		serial_devs[index].ptr_to = 0;

		return (chardev*) &serial_devs[index];
	}
}

inline void init_serial_dev(uint_32 index, uint_32 io_port) {
	// Initialize dev
	serial_devs[index].klass = CLASS_DEV_NONE;
	serial_devs[index].refcount = 0;
	serial_devs[index].flush = 0;
	serial_devs[index].read = 0;
	serial_devs[index].write = 0;
	serial_devs[index].seek = 0;

	//serial_devs[index].ptr_from = 0;
	serial_devs[index].ptr_to = 0;


	serial_devs[index].io_port = io_port;

	// Setup UART
	outb(io_port + PORT_IER, 0); // Disable all interrupts

	outb(io_port + PORT_LCTRL, LC_DLAB);
	outb(io_port + PORT_DATA, 12); // Set bauds to 9600 (115200 / 12)
	outb(io_port + PORT_IER, 0);

	outb(io_port + PORT_LCTRL, LC_BIT8); // 8 bits, no parity, 1 stop bit

//	outb(io_port + PORT_FCTRL, FC_FIFO | FC_CL_RCVFIFO | FC_CL_XMTFIFO | FC_TRIGGER_14); // Buffering (14 bytes)
	outb(io_port + PORT_FCTRL, 0); // No buffering

	outb(io_port + PORT_MCTRL, 0x0B); // IRQs enabled, RTS/DSR set, clear LDAB
	outb(io_port + PORT_IER, IE_RDA | IE_THRE); // Int on RDA or THRE
}

void serial_init() {
	idt_register(ISR_IRQ4, com_1_3_handler, PL_KERNEL);
	pic_clear_irq_mask(4);

	idt_register(ISR_IRQ3, com_2_4_handler, PL_KERNEL);
	pic_clear_irq_mask(3);

	init_serial_dev(0, PORT_1);
	init_serial_dev(1, PORT_2);
}

