#ifndef __DEBUG_H__
#define __DEBUG_H__

#define __mac_xstr(s) #s
#define __mac_str(s) __mac_xstr(s)

#ifdef __KERNEL__

#include <isr.h>

void debug_init(void);

void debug_kernelpanic(registers_t* regs);

void debug_log(const char* message);

void show_eflags(uint_32 eflags);
void show_cs_eip(uint_32 cs, uint_32 eip);
void show_stack(uint_32* esp);
void show_backtrace(uint_32* ebp);

unsigned int symbol_name(uint_32 address, char** string_p);

#include <vga.h>
#include <i386.h>

#ifndef NDEBUG
#define kassert(EXP) { if (!(EXP)) { cli(); vga_printf("\n\\c4cAssertion failed at " \
  __mac_str(__FILE__)":"__mac_str(__LINE__)": "#EXP); hlt(); while(1); } }
#define kassert_verbose(EXP, msg) { if (!(EXP)) { cli(); vga_printf("\n\\c4cAssertion failed at " \
  __mac_str(__FILE__)":"__mac_str(__LINE__)": "#EXP"\n%s\n", msg); hlt(); while(1); } }
#else
#define kassert(EXP) {}
#endif

#else // __TASK__

#define assert(EXP) { if (!(EXP)) { printf("\nAssertion failed at " \
  __mac_str(__FILE__)":"__mac_str(__LINE__)": "#EXP"\n"); } }

#endif

#endif
