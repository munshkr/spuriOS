#include <proc.h>

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
			proc_cpuid_devs[dev].read = 0;
			proc_cpuid_devs[dev].write = 0;
			proc_cpuid_devs[dev].seek = 0;
			proc_cpuid_devs[dev].stream_position = 0;
			return (chardev*) &proc_cpuid_devs[dev];
		}
	}

	return 0;
}
