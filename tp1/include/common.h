#ifndef __COMMON_H__
#define __COMMON_H__

#define ABS(n)  (n > 0 ? n : -n)

#define PL_KERNEL 0
#define PL_USER 3

void memcpy(void* src, void* dst, size_t size);
void memset(void* addr, int value, size_t size);

// Use GCC built-in functionality for variable arguments
#define va_start(v,l) __builtin_va_start(v,l)
#define va_arg(v,l)   __builtin_va_arg(v,l)
#define va_end(v)     __builtin_va_end(v)
#define va_copy(d,s)  __builtin_va_copy(d,s)

typedef __builtin_va_list va_list;

#endif // __COMMON_H__
