#include <pipe.h>
#include <errors.h>
#include <debug.h>
#include <common.h>
#include <lib.h>

#define BUFFER_SIZE 4096
#define MAX_PIPES MAX_FD
#define MAX_ENDPOINTS (MAX_PIPES * 2)

#define C(x) ((pipe_device*) x)
#define PAIR(x) (C(C(x)->pair))
#define IS_READABLE(x) (C(x)->read != NULL)
#define IS_WRITABLE(x) (C(x)->write != NULL)
#define IS_EMPTY(x) (C(self)->bytes_available == 0)
#define IS_FULL(x) (C(self)->bytes_available == BUFFER_SIZE)


typedef struct str_pipe {
	uint_32 klass;
	uint_32 refcount;
	chardev_flush_t* flush;
	chardev_read_t* read;
	chardev_write_t* write;
	chardev_seek_t* seek;

	void* pair;
	sint_8* buffer;

	// XXX `bytes_available` must always be updated on both endpoints
	uint_32 bytes_available;

	uint_32 pos;
	pid queue;
} __attribute__((packed)) pipe_device;

pipe_device endpoints[MAX_ENDPOINTS];


static inline void init_endpoint(pipe_device* p);


void pipe_init(void) {
	debug_log("initializing pipes");

	uint_32 i;
	for (i = 0; i < MAX_ENDPOINTS; i++) {
		endpoints[i].klass = CLASS_DEV_NONE;
	}
}

sint_32 pipe_open(chardev* pipes[2]) {
	bool done = FALSE;
	sint_32 i, in, out;

	for (i = 0, in = -1, out = -1; i < MAX_ENDPOINTS; i++) {
		if (endpoints[i].klass == CLASS_DEV_NONE) {
			if (in < 0) {
				in = i;
			} else if (out < 0) {
				out = i;
				done = TRUE;
				break;
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

	while (sz < size) {
		if (PAIR(self)->klass == CLASS_DEV_NONE) {
			return 0;  // Broken pipe!
		}

		if (IS_EMPTY(self)) {
			loader_enqueue(&(C(self)->queue));
			continue;
		}

		uint_32 eff_size = MIN(C(self)->bytes_available, size - sz);

		if (C(self)->pos + eff_size > BUFFER_SIZE) {
			uint_32 tmp = BUFFER_SIZE - C(self)->pos;
			copy2user(&(C(self)->buffer[C(self)->pos]), buf, tmp);
			copy2user(&(C(self)->buffer[0]), buf, eff_size - tmp);
		} else {
			copy2user(&(C(self)->buffer[C(self)->pos]), buf, eff_size);
		}

		C(self)->pos = (C(self)->pos + eff_size) % BUFFER_SIZE;

		C(self)->bytes_available -= eff_size;
		PAIR(self)->bytes_available -= eff_size;

		sz += eff_size;
	}

	return sz;
}

sint_32 pipe_write(chardev* self, const void* buf, uint_32 size) {
	uint_32 sz = 0;

	while (sz < size) {
		if (PAIR(self)->klass == CLASS_DEV_NONE) {
			return 0;  // Broken pipe!
		}

		if (IS_FULL(self)) {
			loader_enqueue(&(C(self)->queue));
			continue;
		}

		uint_32 eff_size = MIN(BUFFER_SIZE - C(self)->bytes_available, size - sz);

		if (C(self)->pos + eff_size > BUFFER_SIZE) {
			uint_32 tmp = BUFFER_SIZE - C(self)->pos;
			memcpy(buf, &(C(self)->buffer[C(self)->pos]), tmp);
			memcpy(buf, &(C(self)->buffer[0]), eff_size - tmp);
		} else {
			memcpy(buf, &(C(self)->buffer[C(self)->pos]), eff_size);
		}

		C(self)->pos = (C(self)->pos + eff_size) % BUFFER_SIZE;

		C(self)->bytes_available += eff_size;
		PAIR(self)->bytes_available += eff_size;

		sz += eff_size;
	}

	return sz;
}

uint_32 pipe_flush(chardev* self) {
	C(self)->klass = CLASS_DEV_NONE;

	// Unblock all processes waiting on the other endpoint
	pid queue = PAIR(self)->queue;
	while (queue != FREE_QUEUE) {
		loader_unqueue(&queue);
	}

	// Free buffer if both endpoints are gone now
	if (PAIR(self)->klass == CLASS_DEV_NONE) {
		mm_mem_free(C(self)->buffer);
	}

	return 0;
}

sint_32 pipe(sint_32 pipes[2]) {
	chardev* pipe_devs[2];
	sint_32 res = pipe_open(pipe_devs);

	if (res) {
		return res;
	} else {
		pipes[0] = device_descriptor(pipe_devs[0]);
		pipes[1] = device_descriptor(pipe_devs[1]);
		return 0;
	}
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
	p->bytes_available = 0;
	p->pos = 0;
	p->queue = FREE_QUEUE;
	p->pair = NULL;
}
