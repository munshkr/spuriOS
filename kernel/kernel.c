#include "kernel.h"

void kernel_start(void) {
    clear();
	printf("\n\\c0D>\\c0A Registros\n");
	printf("\\c0A  ^^^^^^^^^\n");

    printf("\\c0D> \\c07CR0: \\c0F%x\n", rcr0());
    printf("\\c0D> \\c07CR2: \\c0F%x\n", rcr2());
    printf("\\c0D> \\c07CR3: \\c0F%x\n", rcr3());
    printf("\\c0D> \\c07CR4: \\c0F%x\n", rcr4());

    putchar('\n');
    //test_colors();
}
