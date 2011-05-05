#include <syscalls.h>

#ifdef __KERNEL__

#include <common.h>
#include <idt.h>
#include <loader.h>
#include <mm.h>
#include <vga.h>

static void syscalls_handler(registers_t*);

uint_32 sys_getpid(void);
void sys_exit(void);
void* sys_palloc(void);

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
	  default:
		vga_printf("Invalid system call! Exited");
		sys_exit();
		return;
	}
}

uint_32 sys_getpid() {
	return cur_pid;
}

void sys_exit() {
	loader_exit();
}

void* sys_palloc() {
	void* frame = mm_mem_alloc();

	if (!frame) {
		return NULL;
	}

	// TODO map frame to an empty page
	// of current task PDT
	void* page = NULL;
	// ...

	return page;
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

#endif