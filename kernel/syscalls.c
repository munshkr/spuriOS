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
			regs->eax = (uint_32) palloc();
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
		case SYS_SLEEP:
			sys_sleep(regs);
			break;
		case SYS_OPEN:
			regs->eax = open((const char*) regs->ebx, (uint_32) regs->ecx);
			break;
		case SYS_CLOSE:
			regs->eax = close((int) regs->ebx);
			break;
		case SYS_READ:
			regs->eax = read((int) regs->ebx, (void*) regs->ecx, regs->edx);
			break;
		case SYS_WRITE:
			regs->eax = write((int) regs->ebx, (const void*) regs->ecx, regs->edx);
			break;
		case SYS_SEEK:
			regs->eax = seek((int) regs->ebx, regs->ecx);
			break;
		case SYS_RUN:
			regs->eax = run((const char*) regs->ebx);
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

void sleep(uint_32 time) {
	__asm __volatile("int $0x30" : : "a"(SYS_SLEEP));
}

/* Devices */
int open(const char* filename, uint_32 flags) {
	uint_32 ret;
	__asm __volatile("int $0x30" : "=a"(ret) : "0"(SYS_OPEN), "b" (filename), "c" (flags));
	return ret;
}

int close(int fd) {
	uint_32 ret;
	__asm __volatile("int $0x30" : "=a"(ret) : "0"(SYS_CLOSE), "b" (fd));
	return ret;
}

int read(int fd, void* buf, uint_32 size) {
	uint_32 ret;
	__asm __volatile("int $0x30" : "=a"(ret) : "0"(SYS_READ), "b" (fd), "c" (buf), "d" (size));
	return ret;
}

int write(int fd, const void* buf, uint_32 size) {
	uint_32 ret;
	__asm __volatile("int $0x30" : "=a"(ret) : "0"(SYS_WRITE), "b" (fd), "c" (buf), "d" (size));
	return ret;
}

int seek(int fd, uint_32 size) {
	uint_32 ret;
	__asm __volatile("int $0x30" : "=a"(ret) : "0"(SYS_SEEK), "b" (fd), "c" (size));
	return ret;
}

sint_32 run(const char* filename) {
	uint_32 ret;
	__asm __volatile("int $0x30" : "=a"(ret) : "0"(SYS_RUN), "b" (filename));
	return ret;
}

#endif
