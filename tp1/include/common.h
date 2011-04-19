#ifndef __COMMON_H__
#define __COMMON_H__

#define ABS(n)  (n > 0 ? n : -n)

#define PL_KERNEL 0
#define PL_USER 3

void memcpy(void* src, void* dst, size_t size);
void memset(void* addr, int value, size_t size);

#endif // __COMMON_H__
