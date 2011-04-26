#include <mm.h>
#include <common.h>
#include <i386.h>
#include <debug.h>
#include <vga.h>

#define PAGE_SIZE 4096
#define KERNEL_MEM_BASE 0x100000
#define KERNEL_MEM_END 0x400000
#define MM_KERN_MAP_LEN 96

mmap_entry_t* mmap;
size_t mmap_entries;

// Mapa de bits de memoria del kernel
char mm_kmap[MM_KERN_MAP_LEN];

// --

extern void* _end; // Puntero al fin del c'odigo del kernel.bin (definido por LD).

void* mm_mem_alloc() {
	return NULL;
}

void* mm_mem_kalloc() {
	int dir = 0;
	int i;
	for (i = 0 ; i < MM_KERN_MAP_LEN ; i++) {
		if (get_bit(i) == 0) {
			dir = KERNEL_MEM_BASE + i * PAGE_SIZE;
			set_bit(i);
			break;
		}
	}
	return (void*)dir;
}

void mm_mem_free(void* page) {
	int posicion = ((int)page - KERNEL_MEM_BASE) / PAGE_SIZE;
	clear_bit(posicion);
}

mm_page* mm_dir_new(void) {
	return NULL;
}

void mm_dir_free(mm_page* d) {
}

// --

bool get_bit(int position) {
	int offset = position / 8;
	kassert(offset <= MM_KERN_MAP_LEN);

	char bit = position - offset * 8;
	kassert(bit <= 8);

	return (((mm_kmap[offset] >> (7 - bit)) & 0x01) == 1);
}

void set_bit(int position) {
	int offset = position / 8;
	kassert(offset <= MM_KERN_MAP_LEN);

	char bit = position - offset * 8;
	kassert(bit <= 8);

	char tmp_bit = (1 << (7 - bit));
	mm_kmap[offset] = (mm_kmap[offset] | tmp_bit);
}

void clear_bit(int position) {
	int offset = position / 8;
	kassert(offset <= MM_KERN_MAP_LEN);

	char bit = position - offset * 8;
	kassert(bit <= 8);

	char tmp_bit = ((1 << (7 - bit)) ^ 0xFF);
	mm_kmap[offset] = (mm_kmap[offset] & tmp_bit);
}

void iterate_mmap(void (f)(mmap_entry_t* entry, void* result), void* args) {
	size_t mmap_entry = 0;
	for (mmap_entry = 0; mmap_entry < mmap_entries; mmap_entry++) {
		f(&mmap[mmap_entry], args);
	}
}

void mm_init(mmap_entry_t* mmap_addr, size_t mmap_entries_local) {
	debug_log("initializing memory management");
	memset(&mm_kmap, 0, MM_KERN_MAP_LEN);

	mmap = mmap_addr;
	mmap_entries = mmap_entries_local;
		
}
