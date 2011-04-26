#include <mm.h>
#include <common.h>
#include <i386.h>
#include <debug.h>
#include <vga.h>
#include <mmap.h>

#define PAGE_SIZE 4096
#define HIMEM_BEGIN 0x100000
#define MM_KERNEL_LIMIT 0x400000
#define MM_REQUEST_KERNEL 0
#define MM_REQUEST_USER 1

mmap_entry_t* mmap;
size_t mmap_entries;

// Mapa de bits de memoria
char* mm_bitmap;
int mm_bitmap_byte_len;
// --

extern void* _end; // Puntero al fin del c'odigo del kernel.bin (definido por LD).

void* mm_mem_seek(char request_type) {
	char* cursor = 0;	//
	char* limit = 0;	// Variables definidas para recorrer de a byte el mapa (optimización)
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
		if (*cursor != 0xF) {
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

mm_page* mm_dir_new(void) {
	return NULL;
}

void mm_dir_free(mm_page* d) {
}

// --

bool get_bit(int position) {
	int offset = position / 8;
	kassert(offset <= mm_bitmap_byte_len);

	char bit = position - offset * 8;
	kassert(bit <= 8);

	return (((mm_bitmap[offset] >> (7 - bit)) & 0x01) == 1);
}

void set_bit(int position) {
	int offset = position / 8;
	vga_printf("Off: %d - Len: %d\n", offset, mm_bitmap_byte_len);
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

void mm_print_map() {
	int i = 0;
	char* cursor = mm_bitmap;
	vga_attr.fld.forecolor = 0x3;
	vga_attr.fld.backcolor = 0x3;
	for (i=0 ; i<mm_bitmap_byte_len ; i++, cursor++) {
		vga_putchar(*cursor);
	}
}

void mm_init(mmap_entry_t* mmap_addr, size_t mmap_entries_local) {
	debug_log("initializing memory management");

	// Defino el origen del mapa de memoria
	mm_bitmap = (char*)HIMEM_BEGIN;

	// Variable q almacena el total de memoria del sistema (desde HI-MEM)
	int mem_length = 0;

	// Recorro el mmap para encontrar el último entry válido (desde HI-MEM)
	int ultimo_entry_valido = 0;
	int i;
	mmap_entry_t* entry;
	for (entry = mmap_addr, i = 0; i < mmap_entries_local; ++i, ++entry) {
		if (entry->addr >= HIMEM_BEGIN && entry->type == MMAP_MEMORY_AVAILABLE) {
			ultimo_entry_valido = i;
		}
	}

	// Recorro el mmap para calcular el total de memoria (desde HI-MEM)
	for (entry = mmap_addr, i = 0; i <= ultimo_entry_valido; ++i, ++entry) {
		if (entry->addr >= HIMEM_BEGIN) {
			mem_length += entry->len;
		}
	}

	// Seteo todo el mapa en 0
	int bitmap_byte_len = mem_length / PAGE_SIZE / 8;
	memset(mm_bitmap, 0, bitmap_byte_len);

	// Reservo las páginas de memoria de kernel que usé para el propio mapa
	for (i = 0 ; i < (bitmap_byte_len / PAGE_SIZE) ; i++) {
		set_bit(i);
	}
	if (bitmap_byte_len - i * PAGE_SIZE > 0) {
		set_bit(i);
	}

	// Guardo en la variable global, la cantidad de bytes que ocupa el mapa
	mm_bitmap_byte_len = bitmap_byte_len;

	// Recorro nuevamente el mapa y si encuentro memoria no disponible la marco en el bitmap
	int j;
	int position;
	int length;
	for (entry = mmap_addr, i = 0; i <= ultimo_entry_valido; ++i, ++entry) {
		if (entry->addr >= HIMEM_BEGIN) {
			if (entry->type != MMAP_MEMORY_AVAILABLE) {
				vga_printf("Entry addr: %x\n", (unsigned int)entry->addr);
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
