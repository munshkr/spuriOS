#include <con.h>
#include <vga.h>
#include <mm.h>
#include <common.h>
#include <lib.h>
#include <debug.h>
#include <kbd.h>
#include <pic.h>
#include <idt.h>

#define C(x) ((con_device*) x)
#define MAX_CONSOLES 8


typedef struct str_con_device {
	uint_32 klass;
	uint_32 refcount;
	chardev_flush_t* flush;
	chardev_read_t* read;
	chardev_write_t* write;
	chardev_seek_t* seek;

	pid kbd_queue;

	sint_8* buffer;
	vga_screen_state_t state;

	void* prev;
	void* next;
} __attribute__((packed)) con_device;

static con_device consoles[MAX_CONSOLES];
static con_device* current;


static inline void switch_console(con_device* new_con);
static inline void add_console_after_current(con_device* new_con);
static inline void remove_console(con_device* con);
static inline void draw_empty_frame(void);
static inline void init_keyboard(void);
static inline void wait_byte(void);
static inline uint_8 read_scancode(void);
static inline void update_kbd_state(void);
static void keyboard_handler(registers_t* regs);
//static void print_ring(void);


void con_init(void) {
	debug_log("initializing console");

	uint_32 i;
	for (i = 0; i < MAX_CONSOLES; i++) {
		consoles[i].klass = CLASS_DEV_NONE;
	}
	current = NULL;

	init_keyboard();
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

			// Copy current display buffer if this is the first console
			if (current == NULL) {
				memcpy(vga_addr, c->buffer, PAGE_SIZE);
				c->state = vga_state;
			} else {
				memset(c->buffer, 0, PAGE_SIZE);
				c->state.x = 0;
				c->state.y = 0;
				c->state.attr.vl.vl = VGA_BC_BLACK | VGA_FC_WHITE;
			}

			c->kbd_queue = FREE_QUEUE;

			c->klass = CLASS_DEV_CONSOLE;
			c->refcount = 0;
			c->flush = con_flush;
			c->read = con_read;
			c->write = con_write;
			c->seek = NULL;

			add_console_after_current(c);
			//print_ring();

			switch_console(c);

			return (chardev*) c;
		}
	}

	return NULL;
}

sint_32 con_read(chardev* self, void* buf, uint_32 size) {
	uint_32 sz = 0;
	while (sz < size) {
		loader_enqueue(&C(self)->kbd_queue);

		if (kbd_char_buf) {
			// FIXME A bit of an overkill for just one byte, isn't it?
			copy2user(&kbd_char_buf, buf, 1);
			sz++;
		}
	}
	return sz;
}

sint_32 con_write(chardev* self, const void* buf, uint_32 size) {
	int sz = 0;
	if (C(self) == current) {
		sz = vga_writebuf(vga_addr, &vga_state, buf, size);
		vga_update_cursor();
	} else {
		sz = vga_writebuf(C(self)->buffer, &C(self)->state, buf, size);
	}
	return sz;
}

uint_32 con_flush(chardev* self) {
	remove_console(C(self));
	//print_ring();

	if (C(self) == current) {
		current = NULL;
		if (C(self)->prev == C(self) && C(self)->next == C(self)) {
			draw_empty_frame();
		} else {
			switch_console(C(self)->prev);
		}
	}
	//print_ring();

	mm_mem_free(C(self)->buffer);
	self->klass = CLASS_DEV_NONE;
	return 0;
}


static inline void switch_console(con_device* new_con) {
	if (new_con == current) {
		return;
	}

	if (current) {
		// Save current state
		memcpy(vga_addr, current->buffer, PAGE_SIZE);
		current->state = vga_state;
	}

	// Set focus on new console
	current = new_con;

	// Restore state for new console
	memcpy(current->buffer, vga_addr, PAGE_SIZE);
	vga_state = current->state;
	vga_update_cursor();
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
	if (!(con->prev == con && con->next == con)) {
		C(con->prev)->next = con->next;
		C(con->next)->prev = con->prev;
	}
}

static inline void draw_empty_frame(void) {
	vga_clear();
	vga_reset_pos();
	vga_reset_colors();
	vga_printf("[empty]\n\n");
}

static inline void init_keyboard(void) {
	debug_log("setting up keyboard");
	pic_clear_irq_mask(1);
	idt_register(ISR_IRQ1, keyboard_handler, PL_KERNEL);
}

static void keyboard_handler(registers_t* regs) {
	// FIXME Do this only if there are more than 1 console
	if (current) {
		// TODO Check if a 1-byte buffer is alright with keyboard
		// Maybe we need a bigger buffer?
		wait_byte();
		update_kbd_state();

		if (KBD_ALT_ON && KBD_SHIFT_ON && KBD_LEFT_PRESS) {
			//vga_printf("switching to left...\n");
			switch_console(C(current->prev));
			return;
		}

		if (KBD_ALT_ON && KBD_SHIFT_ON && KBD_RIGHT_PRESS) {
			//vga_printf("switching to right...\n");
			switch_console(C(current->next));
			return;
		}

		// If there is a current console, and there's a task waiting
		// on a `read` from the current console, wake her up.
		if (current->kbd_queue != FREE_QUEUE) {
			loader_unqueue(&current->kbd_queue);
		}
	}
}

static inline void wait_byte(void) {
	uint_8 ctrl;
	for (ctrl = 0; (ctrl & 1) == 0; ctrl = inb(KBD_CTRL_PORT));
}

static inline uint_8 read_scancode(void) {
	return inb(KBD_DATA_PORT);
}

static inline void update_kbd_state(void) {
	kbd_sc_buf = read_scancode();
	//vga_printf("sc: %x\n", kbd_sc_buf);
	kbd_char_buf = 0;

	switch (kbd_sc_buf) {
	case KBD_SC_ESCAPE:
		kbd_escaped = TRUE; break;

	case KBD_SC_LSHIFT:
		kbd_md_state |= KBD_MD_LSHIFT;
		//vga_printf("LSHIFT pressing\n");
		break;
	case KBD_SC_LSHIFT | 0x80:
		kbd_md_state &= ~KBD_MD_LSHIFT;
		//vga_printf("LSHIFT released\n");
		break;

	case KBD_SC_RSHIFT:
		kbd_md_state |= KBD_MD_RSHIFT;
		//vga_printf("RSHIFT pressing\n");
		break;
	case KBD_SC_RSHIFT | 0x80:
		kbd_md_state &= ~KBD_MD_RSHIFT;
		//vga_printf("RSHIFT released\n");
		break;

	case KBD_SC_CTRL:
		kbd_md_state |= (kbd_escaped ? KBD_MD_RCTRL : KBD_MD_LCTRL);
		//vga_printf("CTRL pressing\n");
		break;
	case KBD_SC_CTRL | 0x80:
		kbd_md_state &= ~(kbd_escaped ? KBD_MD_RCTRL : KBD_MD_LCTRL);
		//vga_printf("CTRL released\n");
		break;

	case KBD_SC_ALT:
		kbd_md_state |= (kbd_escaped ? KBD_MD_RALT : KBD_MD_LALT);
		//vga_printf("ALT pressing\n");
		break;
	case KBD_SC_ALT | 0x80:
		kbd_md_state &= (kbd_escaped ? ~KBD_MD_RALT : ~KBD_MD_LALT);
		//vga_printf("ALT released\n");
		break;

	default:
		if (!(kbd_sc_buf & 0x80)) {
			kbd_char_buf = (KBD_SHIFT_ON ? KBD_UPPER_MAP :
			                               KBD_LOWER_MAP)[kbd_sc_buf];
		}
		break;
	}

	// Reset escape state once escaped scancode was received
	if (kbd_escaped && kbd_sc_buf != KBD_SC_ESCAPE) {
		kbd_escaped = FALSE;
	}
}

/*
static void print_ring(void) {
	uint_32 i;
	vga_clear();
	vga_printf("current = %p\n", current);
	for (i = 0; i < MAX_CONSOLES; i++) {
		vga_printf("[%u] %p, ", i, &consoles[i]);
		vga_printf("buf = %p, x = %u, y = %u, prev = %p, next = %p\n",
			consoles[i].buffer, consoles[i].x, consoles[i].y,
			consoles[i].prev, consoles[i].next);
	}
	breakpoint();
}
*/
