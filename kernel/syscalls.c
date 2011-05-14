#include <syscalls.h>

#ifdef __KERNEL__

#include <common.h>
#include <idt.h>
#include <loader.h>
#include <mm.h>
#include <vga.h>
#include <kbd.h>
#include <debug.h>
#include <fs.h>
#include <device.h>

static void syscalls_handler(registers_t*);

uint_32 sys_getpid(void);
void sys_exit(void);
void* sys_palloc(void);
void sys_print (registers_t* regs);
void sys_loc_print (registers_t* regs);
void sys_sleep (registers_t* regs);

void syscalls_init() {
	idt_register(SYS_INT, syscalls_handler, PL_USER);
}

static void syscalls_handler(registers_t* regs) {
	// Syscall number is in EAX
	switch (regs->eax) {
		case SYS_GETPID:
			regs->eax = sys_getpid();
			break;
		case SYS_PALLOC:
			regs->eax = (uint_32) sys_palloc();
			break;
		case SYS_EXIT:
			sys_exit();
			break;
		case SYS_PRINT:
			sys_print(regs);
			break;
		case SYS_LOCPRINT:
			sys_loc_print(regs);
			break;
		case SYS_GETSC:
			regs->eax = (uint_32) sys_getsc();
			break;
		case SYS_SLEEP:
			sys_sleep(regs);
			break;
		case SYS_OPEN:
			regs->eax = open((const char*) regs->ebx, (uint_32) regs->ecx);
			break;
		case SYS_CLOSE:
			regs->eax = (uint_32) close((int) regs->ebx);
			break;
		default:
			vga_printf("Invalid system call! Exited");
			sys_exit();
			break;
	}
	return;
}

uint_32 sys_getpid() {
	return cur_pid;
}

void sys_exit() {
	loader_exit();
}

void* sys_palloc() {
	void* frame = mm_mem_alloc();

	if (!frame) { return NULL; }

	void* page = (void*) processes[cur_pid].next_empty_page_addr;

	mm_map_frame(frame, page, (mm_page*) processes[cur_pid].cr3,
		processes[cur_pid].privilege_level);

	processes[cur_pid].next_empty_page_addr += PAGE_SIZE;

	return page;
}

void sys_sleep(registers_t* regs) {
	// EBP (+0), EIP (+4), time (+8)
	uint_32 time = *(uint_32*)(regs->user_esp + 8);
	loader_sleep(time);
}

void sys_print(registers_t* regs) {
	// EBP (+0), EIP (+4), format (+8), ... args (+12)
	char* format = *(char**)(regs->user_esp + 8);
	uint_32 ret = vga_printf_fixed_args(format, (uint_32*)(regs->user_esp + 12));
	regs->eax = ret;
}

void sys_loc_print(registers_t* regs) {
	//breakpoint();
	// EBP (+0), EIP (+4), row (+8), col (+12), format (+16), ... args (+20)
	uint_32 row = *(uint_32*)(regs->user_esp + 8);
	uint_32 col = *(uint_32*)(regs->user_esp + 12);
	char* format = *(char**)(regs->user_esp + 16);
	uint_32 ret = vga_loc_printf_fixed_args(row, col, format, (uint_32*)(regs->user_esp + 12));
	regs->eax = ret;
}

#else // __TASK__

extern void* syscall_int(int number);

int open(const char* filename, uint_32 flags) {
	uint_32 ret;
	__asm __volatile("int $0x30" : "=a"(ret) : "0"(SYS_OPEN), "b" (filename), "c" (flags));
	return ret;
}

int close(int fd) {
	uint_32 ret;
	__asm __volatile("int $0x30" : "=a"(ret) : "0"(SYS_OPEN), "b" (fd));
	return ret;
}

uint_32 getpid() {
  uint_32 res;
  __asm __volatile("int $0x30" : "=a"(res) : "0"(SYS_GETPID));
	return res;
}

void exit() {
  __asm __volatile("int $0x30" : : "a"(SYS_EXIT));
}

void* palloc() {
  void* res;
  __asm __volatile("int $0x30" : "=a"(res) : "0"(SYS_PALLOC));
	return res;
}

int printf(const char* format, ...) {
	uint_32 ret;
	__asm __volatile("int $0x30" : "=a"(ret) : "0"(SYS_PRINT));
	return ret;
}

int loc_printf(uint_32 row, uint_32 col, const char* format, ...) {
	uint_32 ret;
	__asm __volatile("int $0x30" : "=a"(ret) : "0"(SYS_LOCPRINT));
	return ret;
}

char getsc() {
  uint_32 ret;
  __asm __volatile("int $0x30" : "=a"(ret) : "0"(SYS_GETSC));
	return (char) ret;
}

void sleep(uint_32 time) {
	__asm __volatile("int $0x30" : : "a"(SYS_SLEEP));
}

#endif
