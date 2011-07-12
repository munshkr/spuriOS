#ifndef __SPINLOCK_H__
#define __SPINLOCK_H__

#include <i386.h>
#include <tipos.h>

typedef uint_8 spinlock_t;


LS_INLINE void wait(spinlock_t* spinlock);
extern uint_8 test_and_set(spinlock_t* spinlock);
extern void signal(spinlock_t* spinlock);

LS_INLINE void wait(spinlock_t* spinlock) {
	while (test_and_set(spinlock) == 1) ;
}

#endif // __SPINLOCK_H__
