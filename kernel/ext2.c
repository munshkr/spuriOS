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

// Auxiliar functions

	ext2_bgd* bgdt_entry(ext2_bgd* table, uint_32 index) {
		return (ext2_bgd*)((uint_32)table + index * sizeof(ext2_bgd));
	}

	void print_bgd_entry(ext2_bgd* bgd_entry, uint_32 group_id) {
		vga_printf("BGD Entry for group %d\n", group_id);
		vga_printf("\tblock_bitmap: 0x%x\n", bgd_entry->bg_block_bitmap);
		vga_printf("\tinode_bitmap: 0x%x\n", bgd_entry->bg_inode_bitmap);
		vga_printf("\tinode_table: 0x%x\n", bgd_entry->bg_inode_table);
		vga_printf("\tfree_blocks_count: %d\n", bgd_entry->bg_free_blocks_count);
		vga_printf("\tfree_inodes_count: %d\n", bgd_entry->bg_free_inodes_count);
		vga_printf("\tused_dirs_count: %d\n", bgd_entry->bg_used_dirs_count);
	}

	char hdd_enhanced_buffer[512];	// 512 es por SECTOR_SIZE, pero no se bien donde declararlo

	void hdd_enhanced_read(blockdev* dev, uint_32 starting_byte, void* buffer, uint_32 byte_count) {
		//vga_printf("Llamo hdd_enhanced_read\n");
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
			dev->read(dev, pos, hdd_buf_ptr, dev->size);
			copied_bytes = (dev->size - read_span < byte_count ? dev->size - read_span : byte_count);
			byte_count -= copied_bytes;
			memcpy(hdd_buf_ptr + read_span, buf_ptr, copied_bytes);
			buf_ptr += copied_bytes;
			read_span -= read_span;
			pos++;
			block_count--;
		}
	}

	inline void print_inode(ext2_inode* inode_ptr, uint_32 idx) {
		vga_printf("Inode: %d\n", idx);
		vga_printf("\tmode: %x\n", inode_ptr->i_mode);
		vga_printf("\tuid: %x\n", inode_ptr->i_uid);
		vga_printf("\tsize: %x\n", inode_ptr->i_size);
		/*
		vga_printf("\tacces_time: %x\n", inode_ptr->i_atime);
		vga_printf("\tcreation_time: %x\n", inode_ptr->i_ctime);
		vga_printf("\tmodif_time: %x\n", inode_ptr->i_mtime);
		vga_printf("\tdelete_time: %x\n", inode_ptr->i_dtime);
		*/
		vga_printf("\tgid: %x\n", inode_ptr->i_gid);
		vga_printf("\tlinks_count: %x\n", inode_ptr->i_links_count);
		vga_printf("\tdisk_sectors: %x\n", inode_ptr->i_blocks);
		vga_printf("\tflags: %x\n", inode_ptr->i_flags);
		vga_printf("\tosd1: %x\n", inode_ptr->i_osd1);
		vga_printf("\tblock_list:\n");
		int i;
		for (i = 0; i < 15; i++) {
			if (inode_ptr->i_block[i] != 0) {
				vga_printf("\t\t%d: %x\n", i, inode_ptr->i_block[i]);
			}
		}
		/*
		vga_printf("\tgeneration: %x\n", inode_ptr->i_generation);
		vga_printf("\tfile_acl: %x\n", inode_ptr->i_file_acl);
		vga_printf("\tdir_acl: %x\n", inode_ptr->i_dir_acl);
		vga_printf("\tosd2: %s\n", inode_ptr->i_osd2);
		*/
	}

	inline void print_dir_entry(ext2_dir_entry* dir_entry) {
		char tmp_name[50];
		vga_printf("Dir entry:\n");
		vga_printf("\tinode: %x\n", dir_entry->inode);
		vga_printf("\trec_len: %x\n", dir_entry->rec_len);
		vga_printf("\tname_len: %x\n", dir_entry->name_len);
		vga_printf("\tfile_type: %x\n", dir_entry->file_type);

		memset(tmp_name, 0, 50);
		memcpy(dir_entry->name, tmp_name, dir_entry->name_len);
		vga_printf("\tname: %s\n", tmp_name);
	}

	void obtain_inode(ext2* this, uint_32 inode_id, ext2_inode* inode) {
		uint_32 block_size = 1024 << this->sb->s_log_block_size;
		uint_32 b_group = (inode_id - 1) / this->sb->s_inodes_per_group;
		uint_32 index = (inode_id - 1) % this->sb->s_inodes_per_group;
		uint_32 inode_table = bgdt_entry(this->bgdt, b_group)->bg_inode_table;
		uint_32 inode_starting_byte = inode_table * block_size + index * sizeof(ext2_inode);

		hdd_enhanced_read(this->dev, inode_starting_byte, inode, sizeof(ext2_inode));
	}


// --->


void ext2_init(void) {
}

void ext2_create(ext2* this, blockdev* dev) {
	this->loaded = FALSE;
	this->dev = dev;
	this->sb = NULL;
	this->bgdt = NULL;
	this->bgd_count = 0;
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
	uint_32 b_group_count = this->sb->s_blocks_count / this->sb->s_blocks_per_group;
	if (this->sb->s_blocks_count % this->sb->s_blocks_per_group != 0) {
		b_group_count++;
	}

	hdd_enhanced_read(this->dev, BGDT_START, bgdt, b_group_count * sizeof(ext2_bgd));

	this->bgdt = (ext2_bgd*)bgdt;
	this->bgd_count = b_group_count;

	// Listo, el ext2 está cargado
	this->loaded = TRUE;

	/*
	ext2_bgd* tmp_bgd = this->bgdt;
	vga_printf("\n");
	vga_printf("\\c0FBGDT\n");
	vga_printf("\\c0F----\n");
	for (i = 0; i < b_group_count; i++) {
		print_bgd_entry(tmp_bgd, i);
		tmp_bgd = (ext2_bgd*)((uint_32)tmp_bgd + sizeof(ext2_bgd));
	}
	vga_printf("\n");
	breakpoint();
	*/
}

/*
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
	//print_inode(&mi_inodo, ROOT_DIR_INODE);

	kassert(mi_inodo.i_size <= PAGE_SIZE); // Esto es una prueba, no quiero nada que ocupe más que PAGE_SIZE
	void* tmp_page = mm_mem_kalloc();
	memset(tmp_page, 0, mi_inodo.i_size);
	void* tmp_page_ptr = tmp_page;


	uint_32 tmp_size = mi_inodo.i_size;
	uint_32 bytes_leidos;
	uint_32 inode_block = 0;
	while (tmp_size > 0) {
		bytes_leidos = (tmp_size < block_size ? tmp_size : block_size);
		hdd_enhanced_read(this->dev, mi_inodo.i_block[inode_block] * block_size, tmp_page_ptr, bytes_leidos);
		inode_block++;
		tmp_size -= bytes_leidos;
		tmp_page_ptr += bytes_leidos;
	}

	char tmp_name[50];
	ext2_dir_entry* tmp_entry;
	tmp_size = 0;
	tmp_page_ptr = tmp_page;
	vga_printf("\n");
	while (tmp_size < mi_inodo.i_size) {
		tmp_entry = (ext2_dir_entry*)tmp_page_ptr;
		//print_dir_entry(tmp_entry);
		memset(tmp_name, 0, 50);
		memcpy(tmp_entry->name, tmp_name, tmp_entry->name_len);
		if (tmp_entry->file_type == EXT2_FT_DIR) {
			vga_printf("\\c0A%s\t", tmp_name);
		} else {
			vga_printf("%s\t", tmp_name);
		}
		tmp_size += tmp_entry->rec_len;
		tmp_page_ptr += tmp_entry->rec_len;
	}
	vga_printf("\n");

}
*/

uint_32 str_get_inode(ext2* this, const char* str, uint_32 str_len, uint_32 dir_inode) {
	if (!this->loaded) {
		ext2_finally_create(this);
	}

	// Primero obtengo el inodo del directorio
	uint_32 block_size = 1024 << this->sb->s_log_block_size;
	uint_32 b_group = (dir_inode - 1) / this->sb->s_inodes_per_group;
	uint_32 index = (dir_inode - 1) % this->sb->s_inodes_per_group;
	uint_32 inode_table = bgdt_entry(this->bgdt, b_group)->bg_inode_table;
	uint_32 inode_starting_byte = inode_table * block_size + index * sizeof(ext2_inode);
	ext2_inode mi_inodo;

	hdd_enhanced_read(this->dev, inode_starting_byte, &mi_inodo, sizeof(ext2_inode));
	// --->

	// Segundo traigo el contenido del directorio a una pagina temporal
	kassert(mi_inodo.i_size <= PAGE_SIZE); // Si ocupa más de una página, cagué. Bah, podría arreglarlo (FIXME)
	void* tmp_page = mm_mem_kalloc();
	memset(tmp_page, 0, mi_inodo.i_size); // Dado que ahora voy a copiar del disco, tal vez esto sea al pedo.
	void* tmp_page_ptr = tmp_page;

	uint_32 tmp_size = mi_inodo.i_size;
	uint_32 bytes_leidos;
	uint_32 inode_block = 0;
	while (tmp_size > 0) {
		bytes_leidos = (tmp_size < block_size ? tmp_size : block_size);
		hdd_enhanced_read(this->dev, mi_inodo.i_block[inode_block] * block_size, tmp_page_ptr, bytes_leidos);
		inode_block++;
		tmp_size -= bytes_leidos;
		tmp_page_ptr += bytes_leidos;
	}
	// --->

	// Finalmente, recorro el contenido del directorio buscando mi archivo.
	uint_32 inode_result = 0;
	ext2_dir_entry* tmp_entry;
	tmp_size = 0;
	tmp_page_ptr = tmp_page;
	while (tmp_size < mi_inodo.i_size) {
		tmp_entry = (ext2_dir_entry*)tmp_page_ptr;
		if (tmp_entry->name_len == str_len) {
			if (!strncmp(tmp_entry->name, str, str_len)) {
				inode_result = tmp_entry->inode;
				break;
			}
		}
		tmp_size += tmp_entry->rec_len;
		tmp_page_ptr += tmp_entry->rec_len;
	}
	// --->

	mm_mem_free(tmp_page);
	return inode_result;
}

uint_32 get_file_inode(ext2* this, const char* filename, uint_32 str_len) {
	if (!this->loaded) {
		ext2_finally_create(this);
	}

	kassert(filename[0] == '/');

	uint_32 last_found_inode = ROOT_DIR_INODE;
	uint_32 i_from = 0;
	uint_32 i_to = 0;

	// Swallow the first '/'
	i_to = char_pos(filename, '/', i_from);
	i_from = i_to;

	// If after the first '/' there is nothing more, we have root
	if (i_from == str_len) {
		return last_found_inode;
	}
	// --->

	// Parse the path and for each '/', find the dir or file in the filesystem.
	i_to = char_pos(filename, '/', i_from);
	while(i_to != 0) {
		last_found_inode = str_get_inode(this, filename + i_from, i_to - i_from - 1, last_found_inode);
		if (last_found_inode == 0) {
			return FALSE;
		}

		i_from = i_to;
		i_to = char_pos(filename, '/', i_from);
	}
	//i_to = str_end_pos(filename, i_from);
	i_to = str_len;
	last_found_inode = str_get_inode(this, filename + i_from, i_to - i_from, last_found_inode);
	// --->

	return last_found_inode;
}


uint_32 read_dir(ext2* this, ext2_inode* inode, void* buffer) {
	uint_32 block_size = 1024 << this->sb->s_log_block_size;

	void* tmp_page = mm_mem_kalloc();
	void* tmp_page_ptr = tmp_page;

	uint_32 tmp_size = inode->i_size;
	uint_32 bytes_leidos;
	uint_32 inode_block = 0;
	while (tmp_size > 0) {
		bytes_leidos = (tmp_size < block_size ? tmp_size : block_size);
		hdd_enhanced_read(this->dev, inode->i_block[inode_block] * block_size, tmp_page_ptr, bytes_leidos);
		inode_block++;
		tmp_size -= bytes_leidos;
		tmp_page_ptr += bytes_leidos;
	}

	// --

	ext2_dir_entry* tmp_entry;

	uint_32 local_buffer_offset = 0;
	tmp_size = 0;
	tmp_page_ptr = tmp_page;
	while (tmp_size < inode->i_size) {
		tmp_entry = (ext2_dir_entry*)tmp_page_ptr;
		if (tmp_entry->file_type == EXT2_FT_DIR) {
			memcpy("\\c0A", buffer + local_buffer_offset, 4);
		} else {
			memcpy("\\c07", buffer + local_buffer_offset, 4);
		}
		local_buffer_offset += 4;
		memcpy(tmp_entry->name, buffer + local_buffer_offset, tmp_entry->name_len);
		local_buffer_offset += tmp_entry->name_len;
		memcpy("\t", buffer + local_buffer_offset, 1);
		local_buffer_offset++;

		tmp_size += tmp_entry->rec_len;
		tmp_page_ptr += tmp_entry->rec_len;
	}
	memcpy("\0", buffer + local_buffer_offset, 1);
	local_buffer_offset++;

	return local_buffer_offset;
}

chardev* ext2_open(ext2* this, const char* filename, uint_32 flags) {
	kassert(filename[0] == '/');
	if (!this->loaded) {
		ext2_finally_create(this);
	}
	if (flags & FS_OPEN_WR) {
		return NULL;
	}

	uint_32 tmp_inode = get_file_inode(this, filename, str_end_pos(filename, 0));
	if (tmp_inode == 0) {
		return NULL;
	}

	uint_32 dev;
	for (dev = 0; dev < MAX_EXT2_FILES; dev++) {
		if (ext2_files[dev].klass == CLASS_DEV_NONE) {
			ext2_files[dev].klass = CLASS_DEV_EXT2_FILE;
			ext2_files[dev].refcount = 0;
			ext2_files[dev].flush = ext2_file_flush;
			ext2_files[dev].read = ext2_file_read;
			ext2_files[dev].write = 0;
			ext2_files[dev].seek = ext2_file_seek;

			ext2_files[dev].fs = this;

			obtain_inode(this, tmp_inode, &ext2_files[dev].inode);

			ext2_files[dev].file_size = ext2_files[dev].inode.i_size;
			ext2_files[dev].stream_pos = 0;
			ext2_files[dev].buffer = mm_mem_kalloc();
			ext2_files[dev].buf_pos = 0;
			ext2_files[dev].buf_len = PAGE_SIZE;
			ext2_files[dev].buffer_empty = TRUE;

			if ((ext2_files[dev].inode.i_mode & EXT2_S_IFDIR) != 0) {
				ext2_files[dev].file_size = read_dir(this, &ext2_files[dev].inode, ext2_files[dev].buffer);
				ext2_files[dev].buffer_empty = FALSE;
			}

			return (chardev*) &ext2_files[dev];
		}
	}

	return 0;
}

void e2f_load_buffer(ext2_file* file) {
	uint_32 file_offset = (file->stream_pos / file->buf_len) * file->buf_len;
	uint_32 block_size = 1024 << file->fs->sb->s_log_block_size;

	void* local_buffer_ptr = file->buffer;
	uint_32 tmp_size = file->buf_len;
	uint_32 bytes_leidos;

	uint_32 inode_block = file_offset / block_size;
	while (tmp_size > 0) {
		bytes_leidos = (tmp_size < block_size ? tmp_size : block_size);
		hdd_enhanced_read(file->fs->dev, file->inode.i_block[inode_block] * block_size, local_buffer_ptr, bytes_leidos);
		inode_block++;
		tmp_size -= bytes_leidos;
		local_buffer_ptr += bytes_leidos;
	}

	file->buf_pos = file->stream_pos % file->buf_len;
	file->buffer_empty = FALSE;
}

sint_32 ext2_file_read(chardev* self, void* buf, uint_32 size) {
	if (size == 0) {
		return 0;
	}

	ext2_file* file = (ext2_file*)self;

	if (file->buffer_empty) {
		e2f_load_buffer((ext2_file*)file);
	}

	// Correcciones de lectura
	if (file->stream_pos + size > file->file_size) {
		size = file->file_size - file->stream_pos;
	}
	// --->

	uint_32 remaining_size = size;

	uint_32 bytes_copiados;
	while(remaining_size > 0) {
		// Leo lo que puedo del buffer
		bytes_copiados = (remaining_size < file->buf_len - file->buf_pos ? remaining_size : file->buf_len - file->buf_pos);
		memcpy(file->buffer + file->buf_pos, buf, bytes_copiados);
		file->buf_pos += bytes_copiados;
		buf += bytes_copiados;
		file->stream_pos += bytes_copiados;
		remaining_size -= bytes_copiados;
		// --->

		// Si aún queda lectura pendiente
		if (remaining_size > 0) {	// tal vez haya que hacerlo indistintamente
			e2f_load_buffer((ext2_file*)file);
		}
	}

	return 0;
}

sint_32 ext2_file_seek(chardev* self, uint_32 position) {
	ext2_file* file = (ext2_file*)self;

	if (position > file->file_size) {
		return -EINVALID;
	}

	if (file->stream_pos / file->buf_len != position / file->buf_len) {
		file->stream_pos = position;
		e2f_load_buffer((ext2_file*)file);
	} else {
		file->stream_pos = position;
		file->buf_pos = position % file->buf_len;
		if (file->buffer_empty) {
			e2f_load_buffer((ext2_file*)file);
		}

	}

	return position;
}

uint_32 ext2_file_flush(chardev* self) {
	self->klass = CLASS_DEV_NONE;
	mm_mem_free(((ext2_file*)self)->buffer);
	return 0;
}

