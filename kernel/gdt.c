#include "gdt.h"

gdt_entry gdt[GDT_COUNT] = {
	(gdt_entry){(unsigned int) 0x00000000, (unsigned int) 0x00000000 },

	(gdt_entry){ 
		(unsigned short) 0xFFFF, 
		(unsigned short) 0x0000,
		(unsigned char) 0x00, 
		(unsigned char) 0xA, 
		(unsigned char) 1, 
		(unsigned char) 0, 
		(unsigned char) 1, 
		(unsigned char) 0xF,
		(unsigned char) 0,  
		(unsigned char) 0,  
		(unsigned char) 1,  
		(unsigned char) 0, 
		(unsigned char) 0x00 
	},

	(gdt_entry){ 
		(unsigned short) 0xFFFF, 
		(unsigned short) 0x0000,
		(unsigned char) 0x00, 
		(unsigned char) 0x2, 
		(unsigned char) 1, 
		(unsigned char) 0, 
		(unsigned char) 1, 
		(unsigned char) 0xF,
		(unsigned char) 0,  
		(unsigned char) 0,  
		(unsigned char) 1,  
		(unsigned char) 0, 
		(unsigned char) 0x00 
	},

	(gdt_entry){ 
		(unsigned short) 0x000F, 
		(unsigned short) 0x8000,
		(unsigned char) 0x0B, 
		(unsigned char) 0x2, 
		(unsigned char) 1, 
		(unsigned char) 0, 
		(unsigned char) 1, 
		(unsigned char) 0x0,
		(unsigned char) 0,  
		(unsigned char) 0,  
		(unsigned char) 1,  
		(unsigned char) 0, 
		(unsigned char) 0x00 
	},

	(gdt_entry){ 
		(unsigned short) 0xFFFF, 
		(unsigned short) 0x8000,
		(unsigned char) 0x0B, 
		(unsigned char) 0x2, 
		(unsigned char) 1, 
		(unsigned char) 0, 
		(unsigned char) 1, 
		(unsigned char) 0xF,
		(unsigned char) 0,  
		(unsigned char) 0,  
		(unsigned char) 1,  
		(unsigned char) 0, 
		(unsigned char) 0x00 
	}
};

gdt_descriptor GDT_DESC = {sizeof(gdt)-1, (unsigned int)&gdt};
