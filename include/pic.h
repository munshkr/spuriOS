#ifndef __PIC_H__
#define __PIC_H__

#include <tipos.h>
#include <i386.h>

#define PIC1_PORT 0x20
#define PIC2_PORT 0xA0

#define PIC_EOI 0x20

void pic_reset(uint_8 addr_pic1, uint_8 addr_pic2);
void pic_enable();
void pic_disable();
void pic_set_irq_mask(uint_8 irq);
void pic_clear_irq_mask(uint_8 irq);

/* Prototipos Inline */

#define LS_INLINE static __inline __attribute__((always_inline))

LS_INLINE void pic1_mask(uint_8 mask);
LS_INLINE void pic2_mask(uint_8 mask);
LS_INLINE void pic1_intr_end(void) ;
LS_INLINE void pic2_intr_end(void) ;
LS_INLINE void pic1_intr_end_what(uint_8 irq) ;
LS_INLINE void pic2_intr_end_what(uint_8 irq) ;

/* Funciones Inline */

LS_INLINE void pic1_mask(uint_8 mask) { outb(0x21, mask); } 
LS_INLINE void pic2_mask(uint_8 mask) { outb(0xA1, mask); } 
LS_INLINE void pic1_intr_end(void) { outb(0x20, 0x20); } 
LS_INLINE void pic2_intr_end(void) { outb(0x20, 0x20); outb(0xA0, 0x20); } 
LS_INLINE void pic1_intr_end_what(uint_8 irq) { outb(0x20, 0x60 | (irq & 0x07)); }
LS_INLINE void pic2_intr_end_what(uint_8 irq) { outb(0xA0, 0x60 | (irq & 0x07)); }

#endif
