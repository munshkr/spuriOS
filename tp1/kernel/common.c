#include <tipos.h>

void memcpy(void* src, void* dst, size_t size) {
	int i;
	for (i = 0; i < size; i++) {
		*(uint_8*)dst = *(uint_8*)src;
		src++;
		dst++;
	}
}

void memset(void* addr, int value, size_t size) {
	int i;
	for (i = 0; i < size; i++) {
		*(uint_8*)addr = value;
		addr++;
	}
}
