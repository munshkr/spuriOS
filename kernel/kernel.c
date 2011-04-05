#include "kernel.h"

void kernel_start(void) {
    clear();
    uint32_t cr0 = rcr0();
    printf("%x", cr0);
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
