#include <ext2.h>
#include <debug.h>
#include <device.h>
#include <mm.h>
#include <hdd.h>
#include <lib.h>
#include <fs.h>
#include <errors.h>

#define SUPERBLOCK_START 1024
#define SUPERBLOCK_MAX_LENGTH 1024
#define SUPERBLOCK_MAGIC 0xEF53
#define BGDT_START 2048
#define ROOT_DIR_INODE 2

void ext2_init(void) {
}

ext2_bgd* bgdt_entry(ext2_bgd* table, uint_32 index) {
	return (ext2_bgd*)(table + index * sizeof(ext2_bgd));
}

char hdd_enhanced_buffer[512];

void hdd_enhanced_read(blockdev* dev, uint_32 starting_byte, void* buffer, uint_32 byte_count) {
	breakpoint();
	uint_32 pos = starting_byte / dev->size;
	uint_32 read_span = starting_byte % dev->size;
	uint_32 real_byte_count = (starting_byte % dev->size) + byte_count;
	uint_32 block_count = real_byte_count / dev->size;
	if (real_byte_count % dev->size != 0) {
		block_count++;
	}
	void* buf_ptr = buffer;
	void* hdd_buf_ptr = (void*)hdd_enhanced_buffer;
	uint_32 copied_bytes;
	while(block_count > 0) {
		vga_printf("dev->read(dev, pos:%d, buf:%x, size: %d);\n", pos, (uint_32)hdd_buf_ptr, dev->size);
		dev->read(dev, pos, hdd_buf_ptr, dev->size);
		breakpoint();
		copied_bytes = (dev->size - read_span < byte_count ? dev->size - read_span : byte_count);
		byte_count -= copied_bytes;
		memcpy(hdd_buf_ptr + read_span, buf_ptr, copied_bytes);
		read_span -= read_span;
		pos++;
		block_count--;
	}
}

void ext2_finally_create(ext2* this) {
	void* sb = mm_mem_kalloc();
	memset(sb, 0, PAGE_SIZE);
	this->sb = (ext2_superblock*)sb;

	// Half cabeza: en la misma página que pedí para el superblock, en un offset de 1024 almaceno la bgdt (Block Group Descriptor Table)
	void* bgdt = sb + SUPERBLOCK_MAX_LENGTH;

	// Leo el superblock del disco y lo guardo donde corresponde.
	int i = SUPERBLOCK_MAX_LENGTH / this->dev->size;
	uint_32 pos = SUPERBLOCK_START / this->dev->size;
	void* tmp = sb;
	while(i > 0) {
		this->dev->read(this->dev, pos, tmp, this->dev->size);
		tmp += this->dev->size;
		pos++;
		i--;
	}
	kassert(this->sb->s_magic == SUPERBLOCK_MAGIC);

	// Calculo el tamaño del BGDT, lo copio y actualizo los valores en el struct.
	uint_32 block_count = this->sb->s_blocks_count / this->sb->s_blocks_per_group;

	if (block_count > 0) {
		i = sizeof(ext2_bgd) * block_count / this->dev->size;
		if ((sizeof(ext2_bgd) * block_count) - (i * this->dev->size) > 0) {
			i++;
		}
	} else {
		i = 0;
	}
	pos = BGDT_START / this->dev->size;
	tmp = bgdt;
	while(i > 0) {
		this->dev->read(this->dev, pos, tmp, this->dev->size);
		tmp += this->dev->size;
		pos++;
		i--;
	}

	this->bgdt = (ext2_bgd*) bgdt;
	this->bgd_count = block_count;

	this->loaded = TRUE;

/*
	ext2_bgd* tmp_bgd;
	vga_printf("\n");
	vga_printf("\\c0FBGDT\n");
	vga_printf("\\c0F----\n");
	for (i = 0; i < this->bgd_count; i++) {
		tmp_bgd = bgdt_entry(this->bgdt, i);
		vga_printf("Entry %d\n", i);
		vga_printf("\tblock_bitmap: %x\n", tmp_bgd->bg_block_bitmap);
		vga_printf("\tinode_bitmap: %x\n", tmp_bgd->bg_inode_bitmap);
		vga_printf("\tinode_table: %x\n", tmp_bgd->bg_inode_table);
	}
	vga_printf("\n");
*/
}

void ext2_create(ext2* this, blockdev* dev) {
	this->loaded = FALSE;
	this->dev = dev;
	this->sb = NULL;
	this->bgdt = NULL;
	this->bgd_count = 0;
}

void ext2_read_root(ext2* this, uint_32 flags) {
	if (!this->loaded) {
		ext2_finally_create(this);
	}
	if (flags & FS_OPEN_WR) {
		return;
	}

	uint_32 block_size = 1024 << this->sb->s_log_block_size;
	uint_32 b_group = (ROOT_DIR_INODE - 1) / this->sb->s_inodes_per_group;
	uint_32 index = (ROOT_DIR_INODE - 1) % this->sb->s_inodes_per_group;
	uint_32 inode_table = bgdt_entry(this->bgdt, b_group)->bg_inode_table;

	uint_32 inode_starting_byte = inode_table * block_size + index * sizeof(ext2_inode);
	ext2_inode mi_inodo;

	hdd_enhanced_read(this->dev, inode_starting_byte, &mi_inodo, sizeof(ext2_inode));

	kassert(mi_inodo.i_size <= PAGE_SIZE); // Esto es una prueba, no quiero nada que ocupe más que PAGE_SIZE
	void* tmp_page = mm_mem_kalloc();
	void* tmp_page_ptr = tmp_page;

	uint_32 tmp_size = mi_inodo.i_size;
	uint_32 bytes_leidos;
	uint_32 inode_block = 0;
	while (tmp_size > 0) {
		bytes_leidos = (tmp_size < block_size ? tmp_size : block_size);
		vga_printf("hdd_e_r(dev, s_b:%x, buf:%x, bytes: %d);\n", mi_inodo.i_block[inode_block] * block_size, (uint_32)tmp_page_ptr, bytes_leidos);
		hdd_enhanced_read(this->dev, mi_inodo.i_block[inode_block] * block_size, tmp_page_ptr, bytes_leidos);
		vga_printf("MIRA LA GILADA");
		inode_block++;
		tmp_size -= bytes_leidos;
		tmp_page_ptr += bytes_leidos;
	}

	vga_printf("Dir: %x\n", (uint_32)tmp_page);
	breakpoint();

}

chardev* ext2_open(ext2* this, const char* filename, uint_32 flags) {
	if (!this->loaded) {
		ext2_finally_create(this);
	}
	if (flags & FS_OPEN_WR) {
		return NULL;
	}

	uint_32 block_size = 1024 << this->sb->s_log_block_size;
	uint_32 b_group = (ROOT_DIR_INODE - 1) / this->sb->s_inodes_per_group;
	uint_32 index = (ROOT_DIR_INODE - 1) % this->sb->s_inodes_per_group;
	uint_32 inode_table = bgdt_entry(this->bgdt, b_group)->bg_inode_table;

	uint_32 inode_starting_byte = inode_table * block_size + index * sizeof(ext2_inode);
	ext2_inode mi_inodo;

	hdd_enhanced_read(this->dev, inode_starting_byte, &mi_inodo, sizeof(ext2_inode));

	vga_printf("\nSize: \\c0F%x\n", mi_inodo.i_size);
	vga_printf("\nVal: \\c0F%x\n", mi_inodo.i_block[0] * block_size);

	return NULL;
}

sint_32 ext2_file_read(chardev* self, void* buf, uint_32 size) {
	return 0;
}

sint_32 ext2_file_write(chardev* self, const void* buf, uint_32 size) {
	return 0;
}

uint_32 ext2_file_flush(chardev* self) {
	return 0;
}

