#include <mm.h>
#include <common.h>
#include <lib.h>
#include <i386.h>
#include <debug.h>
#include <vga.h>
#include <mmap.h>
#include <loader.h>
#include <idt.h>

#define PAGE_SIZE 4096
#define HIMEM_BEGIN 0x100000
#define MM_KERNEL_LIMIT 0x400000

#define WITHOUT_ATTRS(x) ((uint_32)x & ~0xFFF)

#define PD_ENTRY_FOR(x) ((uint_32)x >> 22)
#define PT_ENTRY_FOR(x) (((uint_32)x & 0x3FF000) >> 12)

#define PAGE_REQUESTED	2

mmap_entry_t* mmap;
size_t mmap_entries;

mm_page* kernel_pagetable;

// Mapa de bits de memoria
uint_8* mm_bitmap;
int mm_bitmap_byte_len;
// --

static bool shared_with_other_processes(void* vaddr);

extern void* _end; // Puntero al fin del c'odigo del kernel.bin (definido por LD).

void mm_free_page_table_for(void* vaddr, mm_page* pdt) {
	kassert((((uint_32) vaddr) & 0xFFF) == 0);
	mm_page* pd_entry = &pdt[PD_ENTRY_FOR(vaddr)];

	void* frame = (void*) (pd_entry->base << 12);
	mm_mem_free(frame);
	*((uint_32*) pd_entry) = 0;
}

void* mm_vaddr_for_free_pdt_entry(mm_page* pdt) {
	uint_32 entry ;
	for (entry = 0; entry < 1024; entry++) {
		if (!(pdt[entry].attr && MM_ATTR_P)) {
			return (void*)(entry << 22);
		}
	}
	return 0;
}

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
		if ((entry->attr & MM_ATTR_US_U) ||
			(!(entry->attr & MM_ATTR_P) && (*((uint_32*) entry) & PAGE_REQUESTED))) {
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

	uint_32 dir = 0;
	int i;
	for (i = (int)(cursor - mm_bitmap) * 8 ; i < (int)(cursor - mm_bitmap) * 8 + 8 ; i++) {
		if (get_bit(i) == 0) {
			dir = HIMEM_BEGIN + (i * PAGE_SIZE);
			set_bit(i);
			break;
		}
	}

	kassert((dir & 0xfff) == 0);
//	vga_printf("seek, dir %x\n", dir);

	return (void*)dir;
}

void* mm_mem_kalloc() {
	return mm_mem_seek(MM_REQUEST_KERNEL);
}

void* mm_mem_alloc() {
	return mm_mem_seek(MM_REQUEST_USER);
}

void mm_mem_free(void* frame) {
//	vga_printf("mem_free frame %p\n", frame);
	int posicion = ((int)frame - HIMEM_BEGIN) / PAGE_SIZE;
	clear_bit(posicion);
}

// XXX Return type (should not be a pointer?)
mm_page* mm_dir_new(void) {
	mm_page* cr3 = (mm_page*) mm_mem_kalloc();
	memset((void*) cr3, 0, PAGE_SIZE);
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
				void* vaddr = (void*) ((pde << 22) + (pte << 12));
				if ((table[pte].attr & MM_ATTR_P) && !shared_with_other_processes(vaddr)) {
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
	vga_state.attr.fld.forecolor = 0x3;
	vga_state.attr.fld.backcolor = 0x3;
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

bool mm_set_pt_entry(void* vaddr, uint_32 entry_value, mm_page* pdt) {
	kassert((((uint_32) vaddr) & 0xFFF) == 0);
	kassert(!(entry_value & MM_ATTR_P));

	mm_page* pd_entry = &pdt[PD_ENTRY_FOR(vaddr)];

	if (!(pd_entry->attr & MM_ATTR_P)) {
		void* frame = mm_mem_kalloc();
		if (!frame) {
			return FALSE;
		}

		*((uint_32*) pd_entry) = (uint_32) frame;
		pd_entry->attr = MM_ATTR_P | MM_ATTR_RW | MM_ATTR_US_U;
		memset((void*) (pd_entry->base << 12), 0, PAGE_SIZE);
	}

	mm_page* page_table = (mm_page*)(pd_entry->base << 12);
	((uint_32*) page_table)[PT_ENTRY_FOR(vaddr)] = entry_value;

	return TRUE;
}

bool set_as_requested(void* page, mm_page* pdt) {
	kassert((((uint_32) page) & 0xFFF) == 0);
	mm_page* pt_entry = mm_pt_entry_for(page, pdt);
	bool success;
	if (!pt_entry) {
		success = mm_set_pt_entry(page, PAGE_REQUESTED, pdt);
	} else {
		success = mm_set_pt_entry(page, *((uint_32*) pt_entry) | PAGE_REQUESTED, pdt);
	}

	return success;
}

bool is_requested(void* page, mm_page* pdt) {
	mm_page* pt_entry = mm_pt_entry_for(page, pdt);
	if (!pt_entry || (*((uint_32*) pt_entry) & MM_ATTR_P)) {
		return FALSE;
	} else {
		return *((uint_32*) pt_entry) & PAGE_REQUESTED;
	}
}

void* palloc() {
	void* page = (void*) processes[cur_pid].next_empty_page_addr;
	processes[cur_pid].next_empty_page_addr += PAGE_SIZE;

	if (!set_as_requested(page, cur_pdt())) {
		return (void*) 0;
	} else {
		return page;
	}
}

#define PF_PRESENT	1
#define PF_WRITE	2
#define PF_USER		4
#define PF_RSVD		8
#define PF_FETCH	16
static void page_fault_handler(registers_t* regs) {
	if (processes[cur_pid].privilege_level == PL_USER) {
		void* fail_page = (void*)(rcr2() & ~0xFFF);
		if (is_requested(fail_page, cur_pdt())) {
			mm_user_allocate(fail_page);
		} else {
			vga_printf("Invalid %s at vaddr %x on a %s page, process %d. Killed.\n",
				(regs->u.err_code & PF_WRITE ? "write" :
				(regs->u.err_code & PF_FETCH ? "fetch" : "read")),
				rcr2(),
				(regs->u.err_code & PF_PRESENT ? "present" : "non present"),
				cur_pid);
			loader_exit();
		}
	} else {
		debug_kernelpanic(regs);
	}
}

void mm_init(mmap_entry_t* mmap_addr, size_t mmap_entries_local) {
	debug_log("initializing memory management");

	mmap = mmap_addr;
	mmap_entries = mmap_entries_local;

	mm_bitmap = (uint_8*)HIMEM_BEGIN;
	mm_setup_bitmap(mmap_addr, mmap_entries_local);

	mm_init_kernel_pagetable();	

	idt_register(ISR_PGFLT, page_fault_handler, PL_KERNEL);

}

sint_32 mm_share_page(void* vaddr) {
	mm_page* entry = mm_pt_entry_for(vaddr, (mm_page*) processes[cur_pid].cr3);
	if (entry != NULL) {
		if (entry->attr & MM_ATTR_P) {
			entry->attr |= MM_ATTR_USR_SHARED;
			return 0;
		}

		if (entry->attr & PAGE_REQUESTED) {
			mm_user_allocate(vaddr);
			entry->attr |= MM_ATTR_USR_SHARED;
			return 0;
		}
	}

	return -1;
}

void mm_user_allocate(void* vaddr) {
	void* frame = mm_mem_alloc();
	if (!frame) {
		vga_printf("Not enough memory! Killing process %d.\n", cur_pid);
		loader_exit();
	} else {
		mm_map_frame(frame, vaddr, cur_pdt(), PL_USER);
	}
}

static bool shared_with_other_processes(void* vaddr) {
	int i;
	for (i = 0; i < MAX_PID; i++) {
		if (processes[i].id == FREE_PCB_PID || i == cur_pid) {
			continue;
		}

		mm_page* entry = mm_pt_entry_for(vaddr, (mm_page*) processes[i].cr3);

		if (entry != NULL && entry->attr & MM_ATTR_USR_SHARED) {
			return TRUE;
		}
	}
	return FALSE;
}

sint_32 share_page(void* vaddr) {
	return mm_share_page(vaddr);
}
