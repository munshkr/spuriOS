#include <syscalls.h>

#define black		\\c00
#define blue		\\c01
#define green		\\c02
#define cyan		\\c03
#define red			\\c04
#define pink		\\c05
#define orange		\\c06
#define white		\\c07
#define l_black		\\c08
#define l_blue		\\c09
#define l_green		\\c0A
#define l_cyan		\\c0B
#define l_red		\\c0C
#define l_pink		\\c0D
#define l_orange	\\c0E
#define l_white		\\c0F

#define LOGO_ROWS	13
#define LOGO_COLS	43
#define SCREEN_COLS 80
#define SCREEN_ROWS 25

char* spurios_logo[LOGO_ROWS] = {
	" OOOO                          OOOO   OOOO ",
	"OO  OO                        OO   O OO  OO",
	"O    O                        OO   O O    O",
	"OO                            OO   O OO    ",
	" OOO   OOOO   OO  OO OO    OO OO   O  OOO  ",
	"  OOO  OO  OO OO  OO OO OO    OO   O   OOO ",
	"    OO OO  OO OO  OO OOO   OO OO   O     OO",
	"O    O OO  OO OO  OO OO    OO OO   O O    O",
	"OO  OO OO  OO OO  OO OO    OO OO   O OO  OO",
	" OOOO  OOOO    OOOOO OO    OO  OOOO   OOOO ",
	"       OO                                  ",
	"       OO                                  ",
	"OOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOO",
};

char* spurios_logo_compact[LOGO_ROWS] = {
	"\\c0A /--\\                          /--\\   /--\\ ",
	"\\c0A/\\c0C)  (\\c0A|                        /\\c0C)\\c0A   \\ /\\c0C)  (\\c0A|",
	"\\c0A|    |                        ||   | |    |",
	"\\c0A\\\\                            ||   | \\\\    ",
	"\\c0A \\-\\   /--\\   /\\  /\\ /\\    /\\c0C.\\c0A ||   |  \\-\\  ",
	"\\c0A  \\-\\  ||  \\c0C(\\c0A\\ ||  || || -\\c0C)\\c0A    ||   |   \\-\\ ",
	"\\c0A    \\\\ ||  || ||  || |\\c0C.\\c0A/   -- ||   |     \\\\",
	"\\c02|    | ||  || ||  || |/    || ||   | |    |",
	"\\c02|\\c04)  (\\c02/ ||  \\c04(\\c02/ \\\\c04)\\c02  || ||    || \\\\c04)\\c02   / |\\c04)  (\\c02/",
	"\\c02 \\--/  |\\c04o\\c02-/    \\---/ \\/    \\/  \\--/   \\--/ ",
	"\\c02       ||                                  ",
	"\\c02///////||\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\ \\c090x7\\c02 \\\\\\\\",
	"\\c02\\\\\\\\\\\\\\\\///////////////////////////////////",
};

char* spurios_logo_shine[LOGO_ROWS] = {
	"\\c0B /--\\                          /--\\   /--\\ ",
	"\\c0B/)  (|                        /)   \\ /)  (|",
	"\\c0B|    |                        ||   | |    |",
	"\\c0B\\\\                            ||   | \\\\    ",
	"\\c0B \\-\\   /--\\   /\\  /\\ /\\    /. ||   |  \\-\\  ",
	"\\c0B  \\-\\  ||  (\\ ||  || || -)    ||   |   \\-\\ ",
	"\\c0B    \\\\ ||  || ||  || |./   -- ||   |     \\\\",
	"\\c0B|    | ||  || ||  || |/    || ||   | |    |",
	"\\c0B|)  (/ ||  (/ \\)  || ||    || \\)   / |)  (/",
	"\\c0B \\--/  |o-/    \\---/ \\/    \\/  \\--/   \\--/ ",
	"\\c0B       ||                                  ",
	"\\c0B///////||\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\ 0x7 \\\\\\\\",
	"\\c0B\\\\\\\\\\\\\\\\///////////////////////////////////",
};

#define COMET_SMALL_ROWS	20
#define COMET_SMALL_COUNT 	19

char * comet_small[COMET_SMALL_ROWS] = {
	"\\c00 | ",
	"\\c08 | ",
	"\\c08 | ",
	"\\c08 | ",
	"\\c08 | ",
	"\\c08 | ",
	"\\c08 | ",
	"\\c08 | ",
	"\\c08 | ",
	"\\c08 | ",
	"\\c08 | ",
	"\\c08 | ",
	"\\c07 | ",
	"\\c07 | ",
	"\\c07 | ",
	"\\c07 | ",
	"\\c07 | ",
	"   ",
	"\\c0F\\\\c0C.\\c0F/",
	"   ",
};

/*
char* spurios_logo_compact_bkp[LOGO_ROWS] = {
	" /--\                          /--\   /--\ ",
	"/)  (|                        /)   \ /)  (|",
	"|    |                        ||   | |    |",
	"\\                            ||   | \\    ",
	" \-\   /--\   /\  /\ /\    /. ||   |  \-\  ",
	"  \-\  ||  (\ ||  || || -)    ||   |   \-\ ",
	"    \\ ||  || ||  || |./   -- ||   |     \\",
	"|    | ||  || ||  || |/    || ||   | |    |",
	"|)  (/ ||  (/ \)  || ||    || \)   / |)  (/",
	" \--/  |o-/    \---/ \/    \/  \--/   \--/ ",
	"       ||                                  ",
	"       \/                                  ",
	"OOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOO",
};
*/




int main(void) {
	sleep(200);

	//int i;
	//char* c;
	/*
	for (i = 0; i < LOGO_ROWS; i++) {
		c = spurios_logo[i];
		while(*c != 0) {
			if (*c == 'O') {
				*c = (char)0xA;
			}
			c++;
		}
	}
	*/

/*
	for (i = 0; i < LOGO_ROWS; i++) {
		loc_printf(i + 5, 10, spurios_logo_compact[i]);
	}
	*/

	uint_32 i_s = 0;

	uint_32 logo_starting_col = SCREEN_COLS / 2 - LOGO_COLS / 2;

	// Start
	while(i_s < LOGO_ROWS + 5) {
		sleep(200);

		if (i_s < LOGO_ROWS) {
			loc_printf(i_s + 5, logo_starting_col, spurios_logo_shine[i_s]);
		}
		if (i_s > 0 && i_s < LOGO_ROWS + 1) {
			loc_printf(i_s - 1 + 5, logo_starting_col, spurios_logo_compact[i_s - 1]);
		}

		i_s++;
	}

	i_s = 0;

	uint_32 i;
	uint_32 j;
	//bool b_shine = FALSE;

	uint_32 comet_small_idx[COMET_SMALL_COUNT] = {99, 57, 62, 77, 95, 106, 120, 128, 135, 59, 70, 111, 133, 125, 90, 65, 100, 59, 80};
	uint_32 comet_small_col[COMET_SMALL_COUNT] = {26, 58, 14, 18, 50, 2, 66, 10, 30, 42, 70, 6, 54, 22, 62, 38, 46, 74, 34};
	//uint_32 tmp_idx;
	while(1) {
		sleep(50);


		// Cometa Small
			for (i = 0; i < COMET_SMALL_COUNT; i++) {
				if (comet_small_idx[i] < COMET_SMALL_ROWS + SCREEN_ROWS) {
					if (comet_small_idx[i] > COMET_SMALL_ROWS) {
						for (j = 0; j < COMET_SMALL_ROWS; j++) {
							loc_printf(comet_small_idx[i] + j - COMET_SMALL_ROWS, comet_small_col[i], comet_small[j]);
						}
					} else {
						for (j = COMET_SMALL_ROWS - comet_small_idx[i]; j < COMET_SMALL_ROWS; j++) {
							loc_printf(comet_small_idx[i] + j - COMET_SMALL_ROWS, comet_small_col[i], comet_small[j]);
						}
					}
				}
			}
		// --->


		// Logo
			for (i = 0; i < LOGO_ROWS; i++) {
				/*
				if (i_s < LOGO_ROWS) {
					if (i == i_s) {
						loc_alpha_printf(i_s + 5, logo_starting_col, spurios_logo_shine[i_s]);
						b_shine = TRUE;
					}
				}
				if (i_s > 2 && i_s < LOGO_ROWS + 3) {
					if (i == i_s - 3) {
						loc_alpha_printf(i_s - 3 + 5, logo_starting_col, spurios_logo_compact[i_s - 3]);
						b_shine = TRUE;
					}
				}
				if (i_s > LOGO_ROWS && i_s < LOGO_ROWS * 2 + 1) {
					if (i == LOGO_ROWS - (i_s - LOGO_ROWS)) {
						loc_alpha_printf(LOGO_ROWS - (i_s - LOGO_ROWS) + 5, logo_starting_col, spurios_logo_shine[LOGO_ROWS - (i_s - LOGO_ROWS)]);
						b_shine = TRUE;
					}
				}
				if (i_s > LOGO_ROWS + 1 && i_s < LOGO_ROWS * 2 + 2) {
					if (i == LOGO_ROWS - (i_s - LOGO_ROWS) + 1) {
						loc_alpha_printf(LOGO_ROWS - (i_s - LOGO_ROWS) + 6, logo_starting_col, spurios_logo_compact[LOGO_ROWS - (i_s - LOGO_ROWS) + 1]);
						b_shine = TRUE;
					}
				}

				if (!b_shine) {
					*/
					loc_alpha_printf(i + 5, logo_starting_col, spurios_logo_compact[i]);
					/*
				}
				b_shine = FALSE;
				*/
			}
		// --->

		// Logo Shine
			if (i_s < LOGO_ROWS) {
				loc_alpha_printf(i_s + 5, logo_starting_col, spurios_logo_shine[i_s]);
			}
			if (i_s > 2 && i_s < LOGO_ROWS + 3) {
				loc_alpha_printf(i_s - 3 + 5, logo_starting_col, spurios_logo_compact[i_s - 3]);
			}
			if (i_s > LOGO_ROWS && i_s < LOGO_ROWS * 2 + 1) {
				loc_alpha_printf(LOGO_ROWS - (i_s - LOGO_ROWS) + 5, logo_starting_col, spurios_logo_shine[LOGO_ROWS - (i_s - LOGO_ROWS)]);
			}
			if (i_s > LOGO_ROWS + 1 && i_s < LOGO_ROWS * 2 + 2) {
				loc_alpha_printf(LOGO_ROWS - (i_s - LOGO_ROWS) + 6, logo_starting_col, spurios_logo_compact[LOGO_ROWS - (i_s - LOGO_ROWS) + 1]);
			}
		// --->




		i_s++;

		for (i = 0; i < COMET_SMALL_COUNT; i++) {
			comet_small_idx[i]++;
			if (comet_small_idx[i] == 150) {
				comet_small_idx[i] = 0;
			}
		}

		if (i_s == 150) {
			i_s = 0;
		}

		//loc_printf(20, 30, "%d", i_s);
		//printf("HOLA%d", i_s);

	}


/*
	// Copado
	for (i = 0; i < LOGO_ROWS; i++) {
		if (i < 6) {
			printf("\\c0A%s\n", spurios_logo_compact[i]);
		} else {
			if (i == 12) {
				printf("\\c20%s\n", spurios_logo_compact[i]);
			} else {
				printf("\\c02%s\n", spurios_logo_compact[i]);
			}
		}
	}
*/

/*
	for (i = 0; i < LOGO_ROWS; i++) {
		c = spurios_logo[i];
		while(*c != 0) {
			if (*c == 'O') {
				if (i < 6) {
					printf("\\c0AX");
				} else {
					printf("\\c02X");
				}
			} else {
				printf("\\c00 ");
			}
			c++;
		}
		printf("\n");
	}
*/
	//loc_printf(i, end_col / 2 - 4, "\\c08IT IS...");

/*
	printf("\n");
	for (i = 0; i < 127; i++) {
		printf("%c", (char)i);
	}
*/

	return 0;
}


