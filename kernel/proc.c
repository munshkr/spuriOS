#include <proc.h>
#include <common.h>
#include <mm.h>
#include <lib.h>
#include <debug.h>

#define C(x) ((dev_proc_cpuid*) x)

void proc_init() {
	uint_32 dev;
	for (dev = 0; dev < MAX_PROC_CPUID_DEVS; dev++) {
		proc_cpuid_devs[dev].klass = CLASS_DEV_NONE;
	}
}

chardev* proc_cpuid_open() {
	uint_32 dev;
	for (dev = 0; dev < MAX_PROC_CPUID_DEVS; dev++) {
		if (proc_cpuid_devs[dev].klass == CLASS_DEV_NONE) {
			proc_cpuid_devs[dev].klass = CLASS_DEV_PROC_CPUID;
			proc_cpuid_devs[dev].refcount = 0;
			proc_cpuid_devs[dev].flush = 0;
			proc_cpuid_devs[dev].read = proc_cpuid_read;
			proc_cpuid_devs[dev].write = 0;
			proc_cpuid_devs[dev].seek = 0;
			proc_cpuid_devs[dev].stream_position = 0;
		
			sint_32 sz = sprintf((char*) &proc_cpuid_devs[dev].buffer,
				"free kernel mem = %uKB, user = %uKB\n",
				mm_free_page_count(MM_REQUEST_KERNEL) * PAGE_SIZE / 1024,
				mm_free_page_count(MM_REQUEST_USER) * PAGE_SIZE / 1024);
			proc_cpuid_devs[dev].stream_length = sz;
			kassert(sz <= PROC_CPUID_BUFSZ);

			return (chardev*) &proc_cpuid_devs[dev];
		}
	}

	return 0;
}

sint_32 proc_cpuid_read(chardev* self, void* buf, uint_32 size) {
	if (C(self)->stream_position + size > C(self)->stream_length) {
		size = C(self)->stream_length - C(self)->stream_position;
	}

	sint_32 effective_sz = copy2user(
		(void*)(C(self)->buffer + C(self)->stream_position),
		buf, size);

	if (effective_sz > 0) {
		C(self)->stream_position += effective_sz;
	}

	return effective_sz;
}

uint_32 proc_cpuid_flush(chardev* self) {
	self->klass = CLASS_DEV_NONE;
	return 0;
}
