#include <tipos.h>

void memset(void* addr, const uint_32 value, const uint_32 size) {
	uint_32 i;
	for (i = 0; i < size; i++) {
		*(uint_32*)addr++ = value;
	}
}
