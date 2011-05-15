#include <con.h>
#include <vga.h>
#include <mm.h>
#include <common.h>
#include <debug.h>

#define MAX_CONSOLES 8
#define C(x) ((con_device*) x)

typedef struct str_con_device {
	uint_32 klass;
	uint_32 refcount;
	chardev_flush_t* flush;
	chardev_read_t* read;
	chardev_write_t* write;
	chardev_seek_t* seek;

	short* buffer;
	vga_attr_t attr;
	uint_8 x;
	uint_8 y;

	void* prev;
	void* next;
} __attribute__((packed)) con_device;


static con_device consoles[MAX_CONSOLES];
static con_device* current;


static inline void switch_console(con_device* new_con);
static inline void add_console_after_current(con_device* new_con);
static inline void remove_console(con_device* con);
static inline void draw_empty_frame(void);


void con_init(void) {
	uint_32 i;
	for (i = 0; i < MAX_CONSOLES; i++) {
		consoles[i].klass = CLASS_DEV_NONE;
	}
	current = NULL;
}

chardev* con_open(void) {
	uint_32 i;
	for (i = 0; i < MAX_CONSOLES; i++) {
		con_device* c = &consoles[i];
		if (c->klass == CLASS_DEV_NONE) {
			c->buffer = mm_mem_kalloc();
			if (!c->buffer) {
				return NULL;
			}
			memset(c->buffer, 0, PAGE_SIZE);
			c->attr.vl.vl = VGA_BC_BLACK | VGA_FC_WHITE;
			c->x = 0;
			c->y = 0;

			c->klass = CLASS_DEV_CONSOLE;
			c->refcount = 0;
			c->flush = con_flush;
			c->read = con_read;
			c->write = con_write;
			c->seek = NULL;

			add_console_after_current(c);
			switch_console(c);

			return (chardev*) c;
		}
	}

	return NULL;
}

sint_32 con_read(chardev* self, void* buf, uint_32 size) {
	// TODO
	return 0;
}

sint_32 con_write(chardev* self, const void* buf, uint_32 size) {
	// TODO
	return 0;
}

uint_32 con_flush(chardev* self) {
	remove_console(C(self));

	if (C(self) == current) {
		con_device* cur_prev = current->prev;
		con_device* cur_next = current->next;
		current = NULL;

		if (cur_prev == cur_next) {
			draw_empty_frame();
		} else {
			switch_console(cur_prev);
		}
	}

	mm_mem_free(C(self)->buffer);
	self->klass = CLASS_DEV_NONE;
	return 0;
}


static inline void switch_console(con_device* new_con) {
	if (current) {
		// Save current state
		memcpy(vga_addr, current->buffer, PAGE_SIZE);
		current->attr = vga_attr;
		current->x = vga_get_x();
		current->y = vga_get_y();
	}

	// Set focus on new console
	current = new_con;

	// Restore state for new console
	memcpy(current->buffer, vga_addr, PAGE_SIZE);
	vga_attr = current->attr;
	vga_set_pos(current->x, current->y);
}

static inline void add_console_after_current(con_device* new_con) {
	if (current) {
		new_con->prev = current;
		new_con->next = current->next;
		C(current->next)->prev = new_con;
		current->next = new_con;
	} else {
		new_con->prev = new_con;
		new_con->next = new_con;
	}
}

static inline void remove_console(con_device* con) {
	if (con->prev != con->next) {
		C(con->prev)->next = con->next;
		C(con->next)->prev = con->prev;
	}
}

static inline void draw_empty_frame(void) {
	vga_clear();
	vga_reset_pos();
	vga_reset_colors();
	vga_printf("[empty]");
}
