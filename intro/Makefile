export LC_ALL=C

# Programs
OBJDUMP=objdump
OBJCOPY=objcopy
CC=gcc

# Parámetros de programas
NASMFLAGS=-I include/ -g -ggdb -D__KERNEL__
CFLAGS=-m32 -g -ggdb -Wall -Werror -O2 -I include/ \
  -fno-zero-initialized-in-bss -fno-stack-protector -fno-strict-aliasing -ffreestanding -D__KERNEL__
CXXFLAGS=-O2
LDFLAGS=-static -m elf_i386 -nostdlib

# Directorios
DIRBOOT=boot/
DIRKERN=kernel/
DIROUT=bin/

# Boot loader
BIN_BOOT=$(DIRBOOT)floppyboot.bin

# Kernel
OBJS_KERN=kernel/kinit.o kernel/kernel.o kernel/klib.o kernel/gdt.o kernel/video.o
BIN_KERN=$(DIROUT)kernel.bin
DUMP_KERN=$(DIROUT)kernel.bin.asm $(DIROUT)kernel.bin.elf
SYM_KERN=$(DIROUT)kernel.sym

# Imagenes
DISK_LABEL="KERNEL  PSO"
IMG_BASE=$(DIROUT)base.img
IMG_FLOPPY=$(DIROUT)floppy.img

.PHONY: all clean

all: $(IMG_FLOPPY) $(DUMP_KERN) $(SYM_KERN)


# Bootloader - disk
### TODO ###

# Kernel
kernel/%.o: kernel/%.asm
	nasm $(NASMFLAGS) -felf32 -I $(DIRKERN) -o $@ $<

gdt.o: kernel/gdt.c kernel/gdt.h
kernel.o: kernel/kernel.c

$(BIN_KERN).elf: $(OBJS_KERN)
	$(LD) $(LDFLAGS) -N -e start -Ttext 0x1200 -o $@ $(OBJS_KERN) $(LDPSO)

$(BIN_KERN): $(BIN_KERN).elf
	$(OBJCOPY) -S -O binary $< $@

$(BIN_KERN).asm: $(BIN_KERN).elf
	$(OBJDUMP) -S -M intel $< >$@

$(SYM_KERN): $(BIN_KERN).elf
	nm $< | cut -d ' ' -f 1,3 > $@

# Images
$(IMG_BASE):
	@dd if=/dev/zero of=$@ bs=512 count=2880 2>/dev/null
	mkfs.vfat -F 12 -f 2 -n $(DISK_LABEL) $@

$(IMG_FLOPPY): $(BIN_BOOT) $(BIN_KERN) $(IMG_BASE)
	[ -f $(IMG_FLOPPY) ] || dd if=$(IMG_BASE) of=$@ bs=512 count=2880 2>/dev/null
	dd if=$< of=$@ bs=1 count=3 conv=notrunc 2>/dev/null
	dd if=$< of=$@ bs=1 count=450 seek=62 skip=62 conv=notrunc 2>/dev/null
	mcopy -obi $@ $(BIN_KERN) ::kernel.bin

#$(IMG_DISK): ### TODO ###

# Otros
clean:
	rm -f $(BIN_KERN) $(IMG_FLOPPY) $(IMG_BASE) $(OBJS_KERN) $(DUMP_KERN) $(SYM_KERN)

