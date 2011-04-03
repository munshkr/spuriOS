#include "kernel.h"

void kernel_start(void) {
    clear();
    test_colors();
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
