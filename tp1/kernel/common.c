#include <tipos.h>

void memset(uint_32* addr, const uint_32 value, const uint_32 size) {
	uint_32 i;
	for (i = 0; i < size; i++) {
		*addr++ = value;
	}
}
