#include <tipos.h>
#include <common.h>
#include <lib.h>
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
