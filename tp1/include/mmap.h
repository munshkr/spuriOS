#ifndef __MMAP_H__
#define __MMAP_H__

#include "tipos.h"

#define MMAP_MEMORY_AVAILABLE 1
#define MMAP_MEMORY_RESERVED  2

typedef struct mmap_entry {
    uint_64 addr;
    uint_64 len;
    uint_32 type;
    uint_32 acpi;
} __attribute__((__packed__)) mmap_entry_t;


#endif /* __MMAP_H__ */
