#include <syscalls.h>


int main(void) {
	int pid = getpid();

	sint_16 ran0 = 0;
	sint_16 ran1 = 6007;
	sint_16 ran2 = -5532;

	ran1 += ran1 / 124 + ran2 / 135;
	ran2 += ran2 / 632 + ran1 / 22;
	ran0 += ran1 / 15 + ran2 / 26;

	uint_16 end_col = 80;
	uint_16 end_row = 25;
	int i;

	printf("\\c09[%u]\\c07 Do yo know anything about funk?\n", pid);
	sleep(17000);
	printf("\\c09[%u]\\c07 Nothing?\n", pid);
	sleep(17000);
	printf("\\c09[%u]\\c07 I am sure you know something...\n", pid);
	sleep(17000);
	printf("\\c09[%u]\\c07 About funk, of course!\n", pid);
	sleep(17000);
	printf("\\c09[%u]\\c07 No at all?!\n", pid);
	sleep(17000);
	printf("\\c09[%u]\\c07 Well, let me tell you one certain thing that is for sure.\n", pid);
	sleep(17000);
	printf("\\c09[%u]\\c07 The time...\n", pid);
	sleep(17000);
	printf("\\c09[%u]\\c07 You know what time is it?\n", pid);
	sleep(1700);

	for (i = 0; i < end_row / 2; i++) {
		switch (i % 8) {
			case 0:
				loc_printf(i, end_col / 2 - 4, "\\c08IT IS...");
				break;
			case 1:
				loc_printf(i, end_col / 2 - 4, "\\c09IT IS...");
				break;
			case 2:
				loc_printf(i, end_col / 2 - 4, "\\c0AIT IS...");
				break;
			case 3:
				loc_printf(i, end_col / 2 - 4, "\\c0BIT IS...");
				break;
			case 4:
				loc_printf(i, end_col / 2 - 4, "\\c0CIT IS...");
				break;
			case 5:
				loc_printf(i, end_col / 2 - 4, "\\c0DIT IS...");
				break;
			case 6:
				loc_printf(i, end_col / 2 - 4, "\\c0EIT IS...");
				break;
			case 7:
				loc_printf(i, end_col / 2 - 4, "\\c0FIT IS...");
				break;
		}
		sleep(200);
	}

	sleep(2000);

	int j = 0;
	while (j < 600) {
		for (i = 0; i < end_row; i++) {
			ran0 += 1;
			ran1 += 131;
			ran2 += 53;
			if (ran0 > 10000) {
				ran0 -= ran1;
			}
			if (ran1 > 10000) {
				ran1 -= ran2;
			}
			if (ran2 > 10000) {
				ran2 -= ran0;
			}
			switch (ran0 % 16) {
				case 0:
					loc_printf(ran1 % end_row, ran2 % end_col, "\\c00X");
					loc_printf(end_row / 2, end_col / 2 - 7, "\\cF0FUNKY TIME!!!");
					break;
				case 1:
					loc_printf(ran1 % end_row, ran2 % end_col, "\\c01X");
					loc_printf(end_row / 2, end_col / 2 - 7, "\\cE1FUNKY TIME!!!");
					break;
				case 2:
					loc_printf(ran1 % end_row, ran2 % end_col, "\\c02X");
					loc_printf(end_row / 2, end_col / 2 - 7, "\\cD2FUNKY TIME!!!");
					break;
				case 3:
					loc_printf(ran1 % end_row, ran2 % end_col, "\\c03X");
					loc_printf(end_row / 2, end_col / 2 - 7, "\\cC3FUNKY TIME!!!");
					break;
				case 4:
					loc_printf(ran1 % end_row, ran2 % end_col, "\\c04X");
					loc_printf(end_row / 2, end_col / 2 - 7, "\\cB4FUNKY TIME!!!");
					break;
				case 5:
					loc_printf(ran1 % end_row, ran2 % end_col, "\\c05X");
					loc_printf(end_row / 2, end_col / 2 - 7, "\\cA5FUNKY TIME!!!");
					break;
				case 6:
					loc_printf(ran1 % end_row, ran2 % end_col, "\\c06X");
					loc_printf(end_row / 2, end_col / 2 - 7, "\\c96FUNKY TIME!!!");
					break;
				case 7:
					loc_printf(ran1 % end_row, ran2 % end_col, "\\c07X");
					loc_printf(end_row / 2, end_col / 2 - 7, "\\c87FUNKY TIME!!!");
					break;
				case 8:
					loc_printf(ran1 % end_row, ran2 % end_col, "\\c08X");
					loc_printf(end_row / 2, end_col / 2 - 7, "\\c78FUNKY TIME!!!");
					break;
				case 9:
					loc_printf(ran1 % end_row, ran2 % end_col, "\\c09X");
					loc_printf(end_row / 2, end_col / 2 - 7, "\\c69FUNKY TIME!!!");
					break;
				case 10:
					loc_printf(ran1 % end_row, ran2 % end_col, "\\c0AX");
					loc_printf(end_row / 2, end_col / 2 - 7, "\\c5AFUNKY TIME!!!");
					break;
				case 11:
					loc_printf(ran1 % end_row, ran2 % end_col, "\\c0BX");
					loc_printf(end_row / 2, end_col / 2 - 7, "\\c4BFUNKY TIME!!!");
					break;
				case 12:
					loc_printf(ran1 % end_row, ran2 % end_col, "\\c0CX");
					loc_printf(end_row / 2, end_col / 2 - 7, "\\c3CFUNKY TIME!!!");
					break;
				case 13:
					loc_printf(ran1 % end_row, ran2 % end_col, "\\c0DX");
					loc_printf(end_row / 2, end_col / 2 - 7, "\\c2DFUNKY TIME!!!");
					break;
				case 14:
					loc_printf(ran1 % end_row, ran2 % end_col, "\\c0EX");
					loc_printf(end_row / 2, end_col / 2 - 7, "\\c1EFUNKY TIME!!!");
					break;
				case 15:
					loc_printf(ran1 % end_row, ran2 % end_col, "\\c0FX");
					loc_printf(end_row / 2, end_col / 2 - 7, "\\c0FFUNKY TIME!!!");
					break;
			 }
		}
		sleep(100);
		j++;
	}

	printf("\\c09[%u]\\c07 Excellent!\n", pid);
	sleep(10000);
	printf("\\c09[%u]\\c07 Now you know something about funk!\n", pid);
	sleep(10000);
	printf("\\c09[%u]\\c07 Its funky ;)\n", pid);
	sleep(10000);
	printf("\\c09[%u]\\c07 See ya later, alligator! Waahhh!\n", pid);

	return 0;
}


