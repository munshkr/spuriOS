#ifndef __PIPE_H__
#define __PIPE_H__

#include <tipos.h>
#include <device.h>

void pipe_init(void);
sint_32 pipe_open(chardev* pipes[2]);
sint_32 pipe_read(chardev* self, void* buf, uint_32 size);
sint_32 pipe_write(chardev* self, const void* buf, uint_32 size);
uint_32 pipe_flush(chardev* self);

// Syscalls
/*
int pipe(int pipes[2]);
*/

#endif
