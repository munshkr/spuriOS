#ifndef __COMMON_H__
#define __COMMON_H__

#define ABS(n)  (n > 0 ? n : -n)
#define MIN(x, y)  (x > y ? y : x)

#define PL_KERNEL 0
#define PL_USER 3

#define SS_PL(x) (x & 3)

#define SS_K_CODE 0x8
#define SS_K_DATA 0x10
#define SS_U_CODE 0x18
#define SS_U_DATA 0x20
#define SS_TSS 0x28

#define IDLE_PID 0

#define TASK_DEFAULT_EFLAGS 0x202	// IF flag set
#define TASK_U_STACK_ADDRESS 0xffbfe000
#define TASK_K_STACK_ADDRESS 0xffbff000

void memcpy(void* src, void* dst, size_t size);
void memset(void* addr, int value, size_t size);

// Use GCC built-in functionality for variable arguments
#define va_start(v,l) __builtin_va_start(v,l)
#define va_arg(v,l)   __builtin_va_arg(v,l)
#define va_end(v)     __builtin_va_end(v)
#define va_copy(d,s)  __builtin_va_copy(d,s)

typedef __builtin_va_list va_list;

#endif // __COMMON_H__
