#include <common.h>
#include <lib.h>
#include <fs.h>
#include <device.h>
#include <debug.h>

#include <sched.h>
#include <errors.h>

// Discos
#include <fdd.h>
#include <hdd.h>

// Sistemas de archivos
#include <fat12.h>
#include <fat16.h>
#include <ext2.h>

// Dispositivos
#include <serial.h>
#include <con.h>

// Proc
#include <proc.h>

/*
 * Disco 1;
 */
ext2 disk;

/*
 * Se pueden agregar más discos así:
 */
 //~ fat16 disk2;
 //~ fat32 disk3;
 //~ ext2 disk4;
 //~ resiserfs disk5;
 //~ ntfs disk6;

void fs_init(void) {
	/* Inicializar los dispositivos (ojo con las llamadas bloqueantes) */
	// Ejemplo: fat12_create(&disk, fdd_open(0));
	// Ejemplo: fat16_create(&disk2, hdd_open(0));

	debug_log("initializing file system");
	ext2_create(&disk, hdd_open(0));;
}

chardev* fs_open(const char* filename, uint_32 flags) {
	/* Checkea el pedido de apertura como lectura o escritura */
	if ((flags & FS_OPEN_RDWR) == 0) return NULL; /* Pedido frutero */

	if (!strcmp(filename, "/serial0")) return serial_open(0, flags);
	if (!strcmp(filename, "/serial1")) return serial_open(1, flags);
	if (!strcmp(filename, "/console")) return con_open();

	if (!strcmp(filename, "/proc/cpuid")) return proc_cpuid_open();

	if (!strcmp(filename, "/disk/")) ext2_read_root(&disk, flags);
	if (!strncmp(filename, "/disk/", 6)) return ext2_open(&disk, filename+5, flags);

	return NULL;
}

/* Syscalls */
int open(const char* filename, uint_32 flags) {
	chardev* dev = fs_open(filename, flags);
	if (!dev) {
		return ~EINVALID;
	} else {
		fd_t fd = device_descriptor(dev);
		return fd;
	}
}

