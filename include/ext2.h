#ifndef __EXT2_H__
#define __EXT2_H__

#include <tipos.h>
#include <device.h>

//	Superblock
typedef struct str_ext2_superblock {
	uint_32 s_inodes_count;				// Total number of inodes in file system
	uint_32 s_blocks_count;             // Total number of blocks in file system
	uint_32 s_r_blocks_count;           // Number of blocks reserved for Superuser
	uint_32 s_free_blocks_count;        // Total number of unallocated blocks
	uint_32 s_free_inodes_count;        // Total number of unallocated inodes
	uint_32 s_first_data_block;         // Block number of the block containing the superblock
	uint_32 s_log_block_size;           // log2 (block size) - 10. (In other words, the number to shift 1,024 to the left by to obtain the block size)
	uint_32 s_log_frag_size;            // log2 (fragment size) - 10. (In other words, the number to shift 1,024 to the left by to obtain the fragment size)
	uint_32 s_blocks_per_group;         // Number of blocks in each block group
	uint_32 s_frags_per_group;          // Number of fragments in each block group
	uint_32 s_inodes_per_group;         // Number of inodes in each block group
	uint_32 s_mtime;                    // Last mount time (in POSIX time)
	uint_32 s_wtime;                    // Last written time (in POSIX time)
	uint_16 s_mnt_count;                // Number of times the volume has been mounted since its last consistency check (fsck)
	uint_16 s_max_mnt_count;            // Number of mounts allowed before a consistency check (fsck) must be done
	uint_16 s_magic;                    // Ext2 signature (0xef53), used to help confirm the presence of Ext2 on a volume
	uint_16 s_state;                    // File system state
	uint_16 s_errors;                   // What to do when an error is detected
	uint_16 s_minor_rev_level;          // Minor portion of version (combine with Major portion below to construct full version field)
	uint_32 s_lastcheck;                // POSIX time of last consistency check (fsck)
	uint_32 s_checkinterval;            // Interval (in POSIX time) between forced consistency checks (fsck)
	uint_32 s_creator_os;               // Operating system ID from which the filesystem on this volume was created
	uint_32 s_rev_level;                // Major portion of version (combine with Minor portion above to construct full version field)
	uint_16 s_def_resuid;               // User ID that can use reserved blocks
	uint_16 s_def_resgid;               // Group ID that can use reserved blocks
	uint_32 s_first_ino;                // First non-reserved inode in file system. (In versions < 1.0, this is fixed as 11)
	uint_16 s_inode_size;               // Size of each inode structure in bytes. (In versions < 1.0, this is fixed as 128)
	uint_16 s_block_group_nr;           // Block group that this superblock is part of (if backup copy)
	uint_32 s_feature_compat;           // Optional features present (features that are not required to read or write, but usually result in a performance increase. see below)
	uint_32 s_feature_incompat;         // Required features present (features that are required to be supported to read or write. see below)
	uint_32 s_feature_ro_compat;		// Features that if not supported, the volume must be mounted read-only
	uint_8	s_uuid[16];					// File system ID (what is output by blkid)
	uint_8	s_volume_name[16];          // Volume name (C-style string: characters terminated by a 0 byte)
	uint_8	s_last_mounted[64];         // Path volume was last mounted to (C-style string: characters terminated by a 0 byte)
	uint_32 s_algo_bitmap;              // Compression algorithms used (see Required features above)
	uint_8	s_prealloc_blocks;          // Number of blocks to preallocate for files
	uint_8	s_prealloc_dir_blocks;      // Number of blocks to preallocate for directories
	uint_16 alignment;                  // (Unused)
	uint_8	s_journal_uuid[16];         // Journal ID (same style as the File system ID above)
	uint_32 s_journal_inum;             // Journal inode
	uint_32 s_journal_dev;              // Journal device
	uint_32 s_last_orphan;              // Head of orphan inode list
} __attribute__((__packed__)) ext2_superblock;

typedef struct str_ext2_bgd {
	uint_32	bg_block_bitmap;		// Block address of block usage bitmap
	uint_32	bg_inode_bitmap;        // Block address of inode usage bitmap
	uint_32	bg_inode_table;         // Starting block address of inode table
	uint_16	bg_free_blocks_count;   // Number of unallocated blocks in group
	uint_16	bg_free_inodes_count;   // Number of unallocated inodes in group
	uint_16	bg_used_dirs_count;     // Number of directories in group
	uint_16	bg_pad;                 // 16bit value used for padding the structure on a 32bit boundary
	char bg_reserved[12];			// (Unused)
} ext2_bgd;

typedef struct str_ext2_inode {
	uint_16 i_mode;				// Type and Permissions (see below)
	uint_16 i_uid;	 			// User ID
	uint_32 i_size;				// Lower 32 bits of size in bytes
	uint_32 i_atime;			// Last Access Time (in POSIX time)
	uint_32 i_ctime;			// Creation Time (in POSIX time)
	uint_32 i_mtime;			// Last Modification time (in POSIX time)
	uint_32 i_dtime;			// Deletion time (in POSIX time)
	uint_16 i_gid;				// Group ID
	uint_16 i_links_count;		// Count of hard links (directory entries) to this inode. When this reaches 0, the data blocks are marked as unallocated.
	uint_32 i_blocks;			// Count of disk sectors (not Ext2 blocks) in use by this inode, not counting the actual inode structure nor directory entries linking to the inode.
	uint_32 i_flags;			// Flags
	uint_32 i_osd1; 			// Operating System Specific value #1
	uint_32 i_block[15];		// 0 to 11: Direct block pointer n; 12: Singly Indirect Block Pointer; 13: Doubly Indirect Block Pointer; 14: Triply Indirect Block Pointer;
	uint_32 i_generation;		// Generation number (Primarily used for NFS)
	uint_32 i_file_acl;			// In Ext2 version 0, this field is reserved. In version >= 1, Extended attribute block (File ACL).
	uint_32 i_dir_acl;			// In Ext2 version 0, this field is reserved. In version >= 1, Upper 32 bits of file size (if feature bit set) if it's a file, Directory ACL if it's a directory
	uint_32 i_faddr;			// Block address of fragment
	char i_osd2[12];			// Operating System Specific Value #2
} ext2_inode;

typedef struct str_ext2_dir {
	uint_32 inode;
	uint_16 rec_len;
	uint_8 name_len;
	uint_8 file_type;
	char name[];
} ext2_dir;

typedef struct str_ext2 {
	bool loaded;
	blockdev* dev;
	ext2_superblock* sb;
	ext2_bgd* bgdt;
	uint_32 bgd_count;
} ext2;

#define FILENAME_MAX_LENGTH 50
#define MAX_FILES_PER_DIR 50
#define MAX_FILES_PER_FS 200
#define FS_TYPE_FILE 0
#define FS_TYPE_DIR 1

/*
typedef struct str_tree_node {
	char name[FILENAME_MAX_LENGTH];
	uint_8 type;
	tree_node* parent;
	tree_node* childs[MAX_FILES_PER_DIR];
} tree_node;
*/

struct str_ext2_file {
	uint_32 klass;
	uint_32 refcount;
	chardev_flush_t* flush;
	chardev_read_t* read;
	chardev_write_t* write;
	chardev_seek_t* seek;

	pid read_queue;

	bool buffer_free;
	char buffer;

	//pid write_queue;
} __attribute__((packed));

typedef struct str_ext2_file ext2_file;

#define MAX_FILES 32
ext2_file ext2_files[MAX_FILES];



void ext2_init(void);

void ext2_read_root(ext2* this, uint_32 flags);
chardev* ext2_open(ext2* this, const char* filename, uint_32 flags);

void ext2_create(ext2* this, blockdev* dev);

#endif

