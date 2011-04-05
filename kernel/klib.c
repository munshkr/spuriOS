#include "klib.h"
#include "video.h"


LS_INLINE void halt(void) {
    __asm __volatile("hlt");
}

// Bochs magic breakpoint
LS_INLINE void debug(void) {
    __asm __volatile("xchg %bx, %bx");
}

// Read a byte in the specified port
LS_INLINE uint8_t inb(uint16_t port) {
   uint8_t ret;
   __asm __volatile("inb %1, %0" : "=a" (ret) : "dN" (port));
   return ret;
}

// Write a byte out to the specified port
LS_INLINE void outb(uint16_t port, uint8_t value) {
    __asm __volatile("outb %1, %0" : : "dN" (port), "a" (value));
}

LS_INLINE void lcr0(uint32_t value) {
    __asm __volatile("movl %0, %%cr0" : : "r" (value));
}

LS_INLINE uint32_t rcr0(void) {
    uint32_t ret;
    __asm __volatile("movl %%cr0, %0" : "=r" (ret));
    return ret;
}

LS_INLINE uint32_t rcr2(void) {
    uint32_t ret;
    __asm __volatile("movl %%cr2, %0" : "=r" (ret));
    return ret;
}

LS_INLINE uint32_t rcr3(void) {
    uint32_t ret;
    __asm __volatile("movl %%cr3, %0" : "=r" (ret));
    return ret;
}

LS_INLINE uint32_t rcr4(void) {
    uint32_t ret;
    __asm __volatile("movl %%cr4, %0" : "=r" (ret));
    return ret;
}


void test_colors(void) {
    int i, j;
    for (i = 0; i < 16; i++) {
        for (j = 0; j < 16; j++) {
            set_backcolor(i);
            set_forecolor(j);
            printf("COLOR");
        }
    }
}
