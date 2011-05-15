#include <mm.h>
#include <common.h>
#include <i386.h>
#include <debug.h>
#include <vga.h>
#include <mmap.h>

#define PAGE_SIZE 4096
#define HIMEM_BEGIN 0x100000
#define MM_KERNEL_LIMIT 0x400000

#define WITHOUT_ATTRS(x) ((uint_32)x & ~0xFFF)

#define PD_ENTRY_FOR(x) ((uint_32)x >> 22)
#define PT_ENTRY_FOR(x) (((uint_32)x & 0x3FF000) >> 12)

mmap_entry_t* mmap;
size_t mmap_entries;

mm_page* kernel_pagetable;

// Mapa de bits de memoria
uint_8* mm_bitmap;
int mm_bitmap_byte_len;
// --

extern void* _end; // Puntero al fin del c'odigo del kernel.bin (definido por LD).

uint_32 mm_free_page_count(char request_type) {
	uint_32 free_pg_count = 0;

	uint_8* cursor = 0;
	uint_8* limit = 0;
	switch(request_type) {
		case MM_REQUEST_KERNEL:
			cursor = mm_bitmap;
			limit = mm_bitmap + (MM_KERNEL_LIMIT - HIMEM_BEGIN) / PAGE_SIZE / 8;
		break;
		case MM_REQUEST_USER:
			cursor = mm_bitmap + (MM_KERNEL_LIMIT - HIMEM_BEGIN) / PAGE_SIZE / 8;
			limit = mm_bitmap + mm_bitmap_byte_len;
		break;
	}

	for (; cursor < limit ; cursor++) {
		if (*cursor != 0xFF) {
			uint_8 bitfld = ~*cursor;
			while (bitfld != 0) {
				if (bitfld & 1) free_pg_count++;
				bitfld >>= 1;
			}
		}
	}
	return free_pg_count;
}

void* mm_frame_from_page(void* virt_addr, mm_page* pdt) {
	return ((void*)(mm_pt_entry_for(virt_addr, pdt)->base << 12));
}

mm_page* mm_pt_entry_for(void* vaddr, mm_page* pdt) {
	kassert((((uint_32) vaddr) & 0xFFF) == 0);
	mm_page* pd_entry = &pdt[PD_ENTRY_FOR(vaddr)];

	if (!(pd_entry->attr & MM_ATTR_P)) {
		return (mm_page*) 0;
	} else {
		mm_page* page_table = (mm_page*)(pd_entry->base << 12);
		return &(page_table[PT_ENTRY_FOR(vaddr)]);
	}
}

uint_32 mm_pl_of_vaddr(void* vaddr, mm_page* pdt) {
	mm_page* entry = mm_pt_entry_for((void*)((uint_32) vaddr & ~0xFFF), pdt);
	if (!entry) {
		return PL_KERNEL;
	} else {
		if (entry->attr & MM_ATTR_US_U) {
			return PL_USER;
		} else {
			return PL_KERNEL;
		}
	}
}

// TODO Reuse this function when creating new directories
void mm_map_frame(void* phys_addr, void* virt_addr, mm_page* pdt, uint_32 pl) {
	kassert((((uint_32) phys_addr) & 0xFFF) == 0);
	kassert((((uint_32) virt_addr) & 0xFFF) == 0);

	mm_page* pd_entry = &pdt[PD_ENTRY_FOR(virt_addr)];

	if (!(pd_entry->attr & MM_ATTR_P)) {
		// TODO Review all this (kalloc/alloc, User, Supervisor)
		// If not kalloc, fails because page is outside kernel mapping
		*((uint_32*) pd_entry) = (uint_32) mm_mem_kalloc();
		pd_entry->attr = MM_ATTR_P | MM_ATTR_RW | MM_ATTR_US_U;
//			(pl == PL_KERNEL ? MM_ATTR_US_S : MM_ATTR_US_U);
		// Initialize page table
		memset((void*) (pd_entry->base << 12), 0, PAGE_SIZE);
	}

	mm_page* page_table = (mm_page*)(pd_entry->base << 12);
	page_table[PT_ENTRY_FOR(virt_addr)].base = (uint_32)phys_addr >> 12;
	page_table[PT_ENTRY_FOR(virt_addr)].attr = MM_ATTR_P | MM_ATTR_RW |
		(pl == PL_KERNEL ? MM_ATTR_US_S : MM_ATTR_US_U);

	invlpg(virt_addr);
}

void mm_unmap_page(void* virt_addr, mm_page* pdt) {
	kassert((((uint_32) virt_addr) & 0xFFF) == 0);
	
	mm_page* pd_entry = &pdt[PD_ENTRY_FOR(virt_addr)];
	kassert(pd_entry->attr & MM_ATTR_P);

	mm_page* page_table = (mm_page*)(pd_entry->base << 12);
	((uint_32*)page_table)[PT_ENTRY_FOR(virt_addr)] = 0;

	invlpg(virt_addr);
}

void* mm_mem_seek(char request_type) {
	uint_8* cursor = 0;	//
	uint_8* limit = 0;	// Variables definidas para recorrer de a byte el mapa (optimización)
	switch(request_type) {
		case MM_REQUEST_KERNEL:
			cursor = mm_bitmap; // Dirección donde arranca el bitmap
			limit = mm_bitmap + (MM_KERNEL_LIMIT - HIMEM_BEGIN) / PAGE_SIZE / 8; // Dirección hasta donde debe buscar
		break;
		case MM_REQUEST_USER:
			cursor = mm_bitmap + (MM_KERNEL_LIMIT - HIMEM_BEGIN) / PAGE_SIZE / 8; // Dirección donde arranca la sección de usuario del bitmap
			limit = mm_bitmap + mm_bitmap_byte_len; // Fin del bitmap
		break;
	}

	for (; cursor < limit ; cursor++) {
		if (*cursor != 0xFF) {
			break;
		}
	}

	if (cursor == limit) {
		return 0;
	}

	int dir = 0;
	int i;
	for (i = (int)(cursor - mm_bitmap) * 8 ; i < (int)(cursor - mm_bitmap) * 8 + 8 ; i++) {
		if (get_bit(i) == 0) {
			dir = HIMEM_BEGIN + i * PAGE_SIZE;
			set_bit(i);
			break;
		}
	}

	return (void*)dir;
}

void* mm_mem_kalloc() {
	return 	mm_mem_seek(MM_REQUEST_KERNEL);
}

void* mm_mem_alloc() {
	return mm_mem_seek(MM_REQUEST_USER);
}

void mm_mem_free(void* page) {
	int posicion = ((int)page - HIMEM_BEGIN) / PAGE_SIZE;
	clear_bit(posicion);
}

// FIXME Ask about the prototype return type (should not be a pointer?)
mm_page* mm_dir_new(void) {
	mm_page* cr3 = (mm_page*) mm_mem_kalloc();
//	cr3.attr = 0;

	cr3[0].base = (uint_32) kernel_pagetable >> 12;
	cr3[0].attr = MM_ATTR_US_S | MM_ATTR_RW | MM_ATTR_P; 

	return cr3;
}

void mm_dir_free(mm_page* directory) {
	directory = (mm_page*) WITHOUT_ATTRS(directory);
	uint_32 pde, pte;
	for (pde = 1; pde < 1024; pde++) {
		if (directory[pde].attr & MM_ATTR_P) {
			mm_page* table = (mm_page*)(directory[pde].base << 12);
			for (pte = 0; pte < 1024; pte++) {
				if (table[pte].attr & MM_ATTR_P) {
					mm_mem_free((void*)(table[pte].base << 12));
				}
			}
			mm_mem_free((void*) table);
		}
	}
	mm_mem_free((void*) directory);	
}

bool get_bit(int position) {
	int offset = position / 8;
	kassert(offset <= mm_bitmap_byte_len);

	char bit = position - offset * 8;
	kassert(bit <= 8);

	return (((mm_bitmap[offset] >> (7 - bit)) & 0x01) == 1);
}

void set_bit(int position) {
	int offset = position / 8;
	kassert(offset <= mm_bitmap_byte_len);

	char bit = position - offset * 8;
	kassert(bit <= 8);

	char tmp_bit = (1 << (7 - bit));
	mm_bitmap[offset] = (mm_bitmap[offset] | tmp_bit);
}

void clear_bit(int position) {
	int offset = position / 8;
	kassert(offset <= mm_bitmap_byte_len);

	char bit = position - offset * 8;
	kassert(bit <= 8);

	char tmp_bit = ((1 << (7 - bit)) ^ 0xFF);
	mm_bitmap[offset] = (mm_bitmap[offset] & tmp_bit);
}

void iterate_mmap(void (f)(mmap_entry_t* entry, void* result), void* args) {
	size_t mmap_entry = 0;
	for (mmap_entry = 0; mmap_entry < mmap_entries; mmap_entry++) {
		f(&mmap[mmap_entry], args);
	}
}

inline void mm_init_kernel_pagetable() {
	kernel_pagetable = (mm_page*) mm_mem_kalloc();
	kassert(kernel_pagetable->attr == 0);

	uint_32 pte;
	((uint_32*) kernel_pagetable)[0] = 0; // To allow NULL dereferencing 
	for (pte = 1; pte < 1024; pte++) {
		kernel_pagetable[pte].base = pte;
		kernel_pagetable[pte].attr = MM_ATTR_P | MM_ATTR_RW | MM_ATTR_US_S;
	}
}

void mm_print_map() {
	int i = 0;
	uint_8* cursor = mm_bitmap;
	vga_attr.fld.forecolor = 0x3;
	vga_attr.fld.backcolor = 0x3;
	for (i=0 ; i<mm_bitmap_byte_len ; i++, cursor++) {
		vga_putchar(*cursor);
	}
}

inline uint_32 mm_mmap_last_valid_entry(mmap_entry_t* mmap_addr,
	size_t mmap_entries_local) {

	uint_32 last_valid_entry = 0;
	uint_32 i;
	mmap_entry_t* entry;
	for (entry = mmap_addr, i = 0; i < mmap_entries_local; ++i, ++entry) {
		if (entry->addr >= HIMEM_BEGIN && entry->type == MMAP_MEMORY_AVAILABLE) {
			last_valid_entry = i;
		}
	}
	return last_valid_entry;
}

inline uint_32 mm_mem_length(mmap_entry_t* mmap_addr, uint_32 last_valid_entry) {
	uint_32 i, mem_length = 0;
	mmap_entry_t* entry;
	for (entry = mmap_addr, i = 0; i <= last_valid_entry; ++i, ++entry) {
		if (entry->addr >= HIMEM_BEGIN) {
			mem_length += entry->len;
		}
	}
	return mem_length;
}

inline void mm_reserve_bitmap_pages(uint_32 bitmap_byte_len) {
	uint_32 i;
	for (i = 0 ; i < (bitmap_byte_len / PAGE_SIZE) ; i++) {
		set_bit(i);
	}

	if (bitmap_byte_len - i * PAGE_SIZE > 0) {
		set_bit(i);
	}
}

inline void mm_mark_used(mmap_entry_t* mmap_addr, uint_32 last_valid_entry) {
	uint_32 i, j, position, length;
	mmap_entry_t* entry;

	for (entry = mmap_addr, i = 0; i <= last_valid_entry; ++i, ++entry) {
		if (entry->addr >= HIMEM_BEGIN) {
			if (entry->type != MMAP_MEMORY_AVAILABLE) {
				position = (entry->addr - HIMEM_BEGIN) / PAGE_SIZE;
				length = entry->len / PAGE_SIZE;
				if (entry->len - length * PAGE_SIZE > 0) {
					length++;
				}
				for (j = 0 ; j < length ; position++, j++) {
					set_bit(position);
				}
			}
		}
	}
}

inline void mm_setup_bitmap(mmap_entry_t* mmap_addr, size_t mmap_entries_local) {
	uint_32 last_valid_entry = mm_mmap_last_valid_entry(mmap_addr, mmap_entries_local);
	uint_32 mem_length = mm_mem_length(mmap_addr, last_valid_entry);

	mm_bitmap_byte_len = mem_length / PAGE_SIZE / 8;
	memset(mm_bitmap, 0, mm_bitmap_byte_len);

	mm_reserve_bitmap_pages(mm_bitmap_byte_len);
	mm_mark_used(mmap_addr, last_valid_entry);
}

void mm_init(mmap_entry_t* mmap_addr, size_t mmap_entries_local) {
	debug_log("initializing memory management");

	mmap = mmap_addr;
	mmap_entries = mmap_entries_local;

	mm_bitmap = (uint_8*)HIMEM_BEGIN;
	mm_setup_bitmap(mmap_addr, mmap_entries_local);

	mm_init_kernel_pagetable();	
}
