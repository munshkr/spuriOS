#ifndef __MM_H__
#define __MM_H__

#include <tipos.h>
#include <mmap.h>

#define MM_ATTR_P     0x001 // Present
#define MM_ATTR_RW    0x002 // Read/Write
#define MM_ATTR_RW_R  0x000 //
#define MM_ATTR_RW_W  0x002 //
#define MM_ATTR_US    0x004 // User/Supervisor
#define MM_ATTR_US_U  0x004 //
#define MM_ATTR_US_S  0x000 //
#define MM_ATTR_WT    0x008 // Write-Through
#define MM_ATTR_CD    0x010 // Cache Disabled
#define MM_ATTR_A     0x020 // Accessed
#define MM_ATTR_D     0x040 // Dirty (for Pages)
#define MM_ATTR_AVL   0x040 // Available (for Directory)
#define MM_ATTR_PAT   0x080 // Page Table Attribute Index (for Pages)
#define MM_ATTR_SZ_4K 0x000 // Page Size (for Directory)
#define MM_ATTR_SZ_4M 0x080 // Page Size (for Directory)
#define MM_ATTR_G     0x100 // Global (ignored for Directory)

#define MM_ATTR_USR   0xE00 // bits for kernel

#define MM_ATTR_USR_SHARED 0x200
#define MM_ATTR_USR_COR 0x400
//#define MM_ATTR_USR_3 0x800

/* Control Register flags */
#define CR0_PE		0x00000001	// Protection Enable
#define CR0_MP		0x00000002	// Monitor coProcessor
#define CR0_EM		0x00000004	// Emulation
#define CR0_TS		0x00000008	// Task Switched
#define CR0_ET		0x00000010	// Extension Type
#define CR0_NE		0x00000020	// Numeric Errror
#define CR0_WP		0x00010000	// Write Protect
#define CR0_AM		0x00040000	// Alignment Mask
#define CR0_NW		0x20000000	// Not Writethrough
#define CR0_CD		0x40000000	// Cache Disable
#define CR0_PG		0x80000000	// Paging

#define CR4_PCE		0x00000100	// Performance counter enable
#define CR4_MCE		0x00000040	// Machine Check Enable
#define CR4_PSE		0x00000010	// Page Size Extensions
#define CR4_DE		0x00000008	// Debugging Extensions
#define CR4_TSD		0x00000004	// Time Stamp Disable
#define CR4_PVI		0x00000002	// Protected-Mode Virtual Interrupts
#define CR4_VME		0x00000001	// V86 Mode Extensions

#define PAGE_SIZE 4096
typedef struct str_mm_page {
	uint_32 attr:12;
	uint_32 base:20;
}  __attribute__((__packed__, aligned (4))) mm_page;

#define make_mm_entry(base, attr) (mm_page){(uint_32)(attr), (uint_32)(base)}
#define make_mm_entry_addr(addr, attr) (mm_page){(uint_32)(attr), (uint_32)(addr) >> 12}

#define MM_REQUEST_KERNEL 0
#define MM_REQUEST_USER 1

#define MM_TMP_PAGE ((void*) 0xFF800000)

bool mm_set_pt_entry(void* vaddr, uint_32 entry_value, mm_page* pd);

void mm_free_page_table_for(void* vaddr, mm_page* pdt);
void* mm_vaddr_for_free_pdt_entry(mm_page* pdt);

mm_page* mm_pt_entry_for(void* vaddr, mm_page* pdt);
uint_32 mm_pl_of_vaddr(void* vaddr, mm_page* pdt);

uint_32 mm_free_page_count(char request_type);

void mm_map_frame(void* phys_addr, void* virt_addr, mm_page* pdt, uint_32 pl);
void mm_unmap_page(void* virt_addr, mm_page* pdt);

void* mm_frame_from_page(void* virt_addr, mm_page* pdt);

void mm_init(mmap_entry_t* mmap_addr, size_t mmap_entries);
void* mm_mem_seek(char request_type);
void* mm_mem_alloc();
void* mm_mem_kalloc();
void mm_mem_free(void* page);

void mm_user_allocate(void* vaddr);
void mm_copy_on_write(void* vaddr);

/* Manejador de directorios de página */
mm_page* mm_dir_new(void);
void mm_dir_free(mm_page* d);

/* Syscalls */
void* palloc();
sint_32 share_page(void* vaddr);

/* Auxiliares */
void set_bit(int offset);
void clear_bit(int position);
bool get_bit(int position);
void mm_print_map();

#endif
