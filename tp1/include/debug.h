#ifndef __DEBUG_H__
#define __DEBUG_H__

#include <isr.h>

void debug_init(void);

void debug_kernelpanic(const uint_32* stack, const exp_state* expst);


#include <vga.h>
#include <i386.h>

#ifndef NDEBUG
#define __mac_xstr(s) #s
#define __mac_str(s) __mac_xstr(s)
#define kassert(EXP) { if (!(EXP)) { cli(); vga_printf("\n\\c4cAssertion failed at " \
  __mac_str(__FILE__)":"__mac_str(__LINE__)": "#EXP); hlt(); while(1); } }
#else
#define kassert(EXP) {}
#endif

#endif
