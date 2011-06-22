#include <pipe.h>
#include <errors.h>
#include <debug.h>
#include <common.h>

#define C(x) ((pipe_device*) x)
#define MAX_PIPES MAX_FD

#define BUFFER_SIZE 4096
#define MAX_ENDPOINTS (MAX_PIPES * 2)


typedef struct str_pipe {
	uint_32 klass;
	uint_32 refcount;
	chardev_flush_t* flush;
	chardev_read_t* read;
	chardev_write_t* write;
	chardev_seek_t* seek;

	sint_8* buffer;
	uint_32 read_pos;
	uint_32 write_pos;
	pid read_queue;
	pid write_queue;

	void* pair;
} __attribute__((packed)) pipe_device;

pipe_device endpoints[MAX_ENDPOINTS];


static inline void init_endpoint(pipe_device* p);
static inline uint_32 bytes_available(pipe_device* p);


void pipe_init(void) {
	debug_log("initializing pipes");

	uint_32 i;
	for (i = 0; i < MAX_ENDPOINTS; i++) {
		endpoints[i].klass = CLASS_DEV_NONE;
	}
}

sint_32 pipe_open(chardev* pipes[2]) {
	bool done = FALSE;
	uint_32 i, in, out;

	for (i = 0, in = -1, out = -1; done || (i < MAX_ENDPOINTS); i++) {
		if (endpoints[i].klass == CLASS_DEV_NONE) {
			if (in < 0) {
				in = i;
			} else if (out < 0) {
				out = i;
				done = TRUE;
			}
		}
	}
	if (!done) return -ENOMEM;

	kassert(in >= 0 && in < MAX_ENDPOINTS);
	kassert(out >= 0 && in < MAX_ENDPOINTS);

	sint_8* buffer = mm_mem_kalloc();
	if (!buffer) return -ENOMEM;

	pipe_device* p = &endpoints[in];
	init_endpoint(p);
	p->buffer = buffer;
	p->read = pipe_read;
	p->pair = (void*) &endpoints[out];
	pipes[0] = (chardev*) p;

	p = &endpoints[out];
	init_endpoint(p);
	p->buffer = buffer;
	p->write = pipe_write;
	p->pair = (void*) &endpoints[in];
	pipes[1] = (chardev*) p;

	return 0;
}

sint_32 pipe_read(chardev* self, void* buf, uint_32 size) {
	uint_32 sz = 0;
	uint_32 available;

	while (sz < size) {
		available = bytes_available(C(self));
		if (!available) {
			loader_enqueue(&(C(self)->read_queue));

			if (self->klass == CLASS_DEV_NONE) {
				return 0;  // Broken pipe!
			}
			continue;
		}

		uint_32 eff_size = MIN(available, size - sz);

		if (C(self)->read_pos + eff_size > BUFFER_SIZE) {
			uint_32 tmp = BUFFER_SIZE - C(self)->read_pos;
			copy2user(&(C(self)->buffer[C(self)->read_pos]), buf, tmp);
			copy2user(&(C(self)->buffer[0]), buf, eff_size - tmp);
		} else {
			copy2user(&(C(self)->buffer[C(self)->read_pos]), buf, eff_size);
		}
		C(self)->read_pos = (C(self)->read_pos + eff_size) % BUFFER_SIZE;
		sz += eff_size;
	}

	return sz;
}

sint_32 pipe_write(chardev* self, const void* buf, uint_32 size) {
	return 0;
}

uint_32 pipe_flush(chardev* self) {
	return 0;
}

static inline void init_endpoint(pipe_device* p) {
	kassert(p->klass == CLASS_DEV_NONE);

	p->klass = CLASS_DEV_PIPE;
	p->refcount = 0;
	p->flush = pipe_flush;
	p->read = NULL;
	p->write = NULL;
	p->seek = NULL;

	p->buffer = NULL;
	p->read_pos = 0;
	p->write_pos = 0;
	p->read_queue = FREE_QUEUE;
	p->write_queue = FREE_QUEUE;
	p->pair = NULL;
}

static inline uint_32 bytes_available(pipe_device* p) {
	kassert(p->read_pos < BUFFER_SIZE);
	kassert(p->write_pos < BUFFER_SIZE);

	uint_32 bytes = p->write_pos - p->read_pos;
	if (p->read_pos > p->write_pos) {
		bytes += BUFFER_SIZE - 1;
	}

	return bytes;
}
