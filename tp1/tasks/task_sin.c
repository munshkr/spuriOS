#include <syscalls.h>


int main(void) {
	uint_16 init_col = 20;
	uint_16 init_row = 15;
	uint_16 end_col = 30;
	uint_16 end_row = 20;
	int i;
	sint_16 offset = end_row - init_row;
	sint_16 res_row;
	uint_8 color = 0;

	while (1) {
		for (i = init_col; i < end_col; i++) {
			res_row = end_row - i - offset;
			if (res_row <= end_row && res_row >= init_row) {
				switch (color % 8) {
					case 0:
						loc_printf(res_row, i, "\\c04X");
						break;
					case 1:
						loc_printf(res_row, i, "\\c05X");
						break;
					case 2:
						loc_printf(res_row, i, "\\c06X");
						break;
					case 3:
						loc_printf(res_row, i, "\\c07X");
						break;
					case 4:
						loc_printf(res_row, i, "\\c08X");
						break;
					case 5:
						loc_printf(res_row, i, "\\c09X");
						break;
					case 6:
						loc_printf(res_row, i, "\\c0AX");
						break;
					case 7:
						loc_printf(res_row, i, "\\c0BX");
						break;
				 }
			}
		}
		offset--;
		if (offset <= -(end_col)) {
			offset = end_row - init_row;
			color++;
			color = color % 8;
		}
		sleep(100);
	}


	return 0;
}

