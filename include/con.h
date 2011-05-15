#ifndef __CON_H__
#define __CON_H__

#include <tipos.h>
#include <device.h>

void con_init(void);

chardev* con_open(void);
sint_32 con_read(chardev* this, void* buf, uint_32 size);
sint_32 con_write(chardev* this, const void* buf, uint_32 size);
uint_32 con_flush(chardev* this);

#endif
