#ifndef __ISR_H__
#define __ISR_H__

#include <tipos.h>

typedef struct registers
{
    uint_32 ds;                  // Data segment selector
    uint_32 edi, esi, ebp, esp, ebx, edx, ecx, eax; // Pushed by pusha
    uint_32 int_no;              // Interrupt number
    union {
      uint_32 err_code;          // Error code (if applicable)
      uint_32 irq;               // IRQ number (if we are in an IRQ handler)
    } u;
    uint_32 eip, cs, eflags, useresp, ss; // Pushed by the processor
} __attribute__((__packed__)) registers_t;

typedef void (*isr_t)(registers_t);

extern isr_t interrupt_handlers[];

#endif /* __ISR_H__ */
