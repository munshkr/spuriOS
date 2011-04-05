#include "kernel.h"

void kernel_start(void) {
    clear();

    printf("CR0: %x\n", rcr0());
    printf("CR2: %x\n", rcr2());
    printf("CR3: %x\n", rcr3());
    printf("CR4: %x\n", rcr4());

    putchar('\n');
    test_colors();
}
