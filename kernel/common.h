#ifndef __COMMON_H__
#define __COMMON_H__


#define ABS(n)  (n > 0 ? n : -n)

typedef unsigned long long uint64_t;
typedef unsigned int       uint32_t;
typedef unsigned short     uint16_t;
typedef unsigned char      uint8_t;
typedef uint32_t bool;

typedef long long int64_t;
typedef int       int32_t;
typedef short     int16_t;
typedef char      int8_t;

#define LS_INLINE static __inline __attribute__((always_inline))

static inline void halt(void) {
    __asm __volatile("hlt");
}

// Bochs magic breakpoint
static inline void debug(void) {
    __asm __volatile("xchg %bx, %bx");
}

// Read a byte in the specified port
static inline uint8_t inb(uint16_t port) {
   uint8_t ret;
   __asm __volatile("inb %1, %0" : "=a" (ret) : "dN" (port));
   return ret;
}

// Write a byte out to the specified port
static inline void outb(uint16_t port, uint8_t value) {
    __asm __volatile("outb %1, %0" : : "dN" (port), "a" (value));
}

LS_INLINE void lcr0(uint32_t value) {
  __asm __volatile("movl %0 %%cr0" : : "r" (value));
}

LS_INLINE uint32_t rcr0(void) {
  uint32_t value;
  __asm __volatile("movl %%cr0, %0" : "=r" (value));
  return value;
}


#endif /* __COMMON_H__ */
