#ifndef __TIMER_H__
#define __TIMER_H__

#include <tipos.h>

void timer_init(uint_32 frequency);
void timer_draw_clock(void);

extern uint_32 tick;

#endif // __TIMER_H__
