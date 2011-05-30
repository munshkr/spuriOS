#include <tipos.h>
#include <common.h>
#include <mm.h>
#include <loader.h>

sint_32 copy2user(void* src, void* dst_usr, size_t size) {
	uint_32 pl = mm_pl_of_vaddr(dst_usr, cur_pdt());
	if (pl == PL_KERNEL) {
		return -1;
	} else {
		memcpy(src, dst_usr, size);
		return size;
	}
}

// TODO Move to lib.c (it can be reused in user tasks!)
void memcpy(void* src, void* dst, size_t size) {
	int i;
	for (i = 0; i < size; i++) {
		*(uint_8*)dst = *(uint_8*)src;
		src++;
		dst++;
	}
}

// TODO Move to lib.c (it can be reused in user tasks!)
void memset(void* addr, int value, size_t size) {
	int i;
	for (i = 0; i < size; i++) {
		*(uint_8*)addr = value;
		addr++;
	}
}
