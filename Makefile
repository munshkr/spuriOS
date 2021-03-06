export LC_ALL=C

# Programs
OBJDUMP=objdump
OBJCOPY=objcopy
CC=gcc
MKFS=/sbin/mkfs.vfat

# Parámetros de programas
NASMFLAGS=-I include/ -g -ggdb -D__KERNEL__

CFLAGS=-m32 -g -ggdb -Wall -Werror -Wdisabled-optimization -I include/ \
  -fno-zero-initialized-in-bss -fno-stack-protector -fno-strict-aliasing -ffreestanding -D__KERNEL__

#CFLAGS=-m32 -g -ggdb -Wall -Werror -O2 -I include/ \
  -fno-zero-initialized-in-bss -fno-stack-protector -fno-strict-aliasing -ffreestanding -D__KERNEL__
#CXXFLAGS=-O2
#CXXFLAGS=-Wdisabled-optimization

LDFLAGS=-static -m elf_i386 -nostdlib

# Directorios
DIRBOOT=boot/
DIRKERN=kernel/
DIROUT=bin/
DIRTMP=/tmp/
DIRDOC=doc/

# Boot loader
BIN_BOOT=$(DIRBOOT)floppyboot.bin

# Kernel
OBJS_KERN=kernel/kinit.o \
	kernel/a20.o \
	kernel/common.o \
	kernel/con.o \
	kernel/debug.o \
	kernel/device.o \
	kernel/fat12.o \
	kernel/fat16.o \
	kernel/fdd.o \
	kernel/fs.o \
	kernel/gdt.o \
	kernel/hdd.o \
	kernel/idt.o \
	kernel/interrup.o \
	kernel/isr.o \
	kernel/kbd.o \
	kernel/kernel.o \
	kernel/lib.o \
	kernel/loader.o \
	kernel/mmap.o \
	kernel/mm.o \
	kernel/pic.o \
	kernel/proc.o \
	kernel/sched.o \
	kernel/sem.o \
	kernel/serial.o \
	kernel/syscalls.o \
	kernel/tasks.o \
	kernel/taskswitch.o \
	kernel/timer.o \
	kernel/vga.o \
	kernel/ext2.o \
	kernel/pipe.o \

BIN_KERN=$(DIROUT)kernel.bin
DUMP_KERN=$(DIROUT)kernel.bin.asm $(DIROUT)kernel.bin.elf $(DIROUT)kernel.bin.orig.elf $(DIROUT)kernel.bin.dbg
SYM_KERN=$(DIROUT)kernel.sym
SYMBOLS=$(DIROUT)symbols
SYMBOLS_FILES=$(SYMBOLS).asm $(SYMBOLS)_null.asm $(SYMBOLS).o $(SYMBOLS)_null.o

# Tareas
TASKS=\
	tasks/init.pso \
	tasks/console.pso \
	tasks/serial.pso \
	tasks/screen_saver.pso \
	tasks/fork.pso \
	tasks/pipe.pso \
	tasks/shared_mem.pso \
	tasks/krypt.pso \
	tasks/lazy.pso \
	tasks/memkrypt.pso \
	tasks/copy_on_write.pso \
	tasks/unkrypt.pso \

TASKS_ELF:=$(TASKS:.pso=.elf)
OBJS_TASKS:=$(TASKS:.pso=.o) tasks/pso_head.o tasks/pso_tail.o tasks/syscalls.o tasks/lib.o

# Tests
TESTS=\
	tasks/palloc.pso \
	tasks/getpid.pso \
	tasks/ut_con.pso \
	tasks/cpuid.pso \
	tasks/cp2user.pso \

TESTS_ELF:=$(TESTS:.pso=.elf)
OBJS_TESTS:=$(TESTS:.pso=.o) tasks/pso_head.o tasks/pso_tail.o tasks/syscalls.o tasks/lib.o

# Imagenes
DISK_LABEL="KERNEL  PSO"
IMG_BASE=$(DIROUT)base.img
IMG_FLOPPY=$(DIROUT)floppy.img
IMG_HDD=$(DIROUT)hdd.img

# Documents
DOC_FILES=$(DIRDOC)tp3-docs.pdf


.PHONY: all clean

all: $(IMG_FLOPPY) $(DUMP_KERN) $(SYM_KERN) $(IMG_HDD)

no_disk: $(IMG_FLOPPY) $(DUMP_KERN) $(SYM_KERN)

# Bootloader - disk
### TODO ###

# Kernel
kernel/%.o: kernel/%.asm
	nasm $(NASMFLAGS) -felf32 -I $(DIRKERN) -o $@ $<

kernel/tasks.o: kernel/tasks.asm $(TASKS) $(TASKS_ELF) $(TESTS) $(TESTS_ELF)
	nasm $(NASMFLAGS) -felf32 -I $(DIRKERN) -o $@ $<

tasks/pso_head.o: tasks/pso_head.asm
	nasm $(NASMFLAGS:__KERNEL__=__TASK__) -felf32 -I $(DIRKERN) -o $@ $<

tasks/pso_tail.o: tasks/pso_tail.asm
	nasm $(NASMFLAGS:__KERNEL__=__TASK__) -felf32 -I $(DIRKERN) -o $@ $<

tasks/syscalls.o: kernel/syscalls.c
	$(CC) $(CFLAGS:__KERNEL__=__TASK__) -c -o $@ $<

tasks/lib.o: kernel/lib.c
	$(CC) $(CFLAGS:__KERNEL__=__TASK__) -c -o $@ $<

tasks/%.pso: tasks/%.elf
	$(OBJCOPY) -S -O binary $< $@

tasks/%.o: tasks/%.c
	$(CC) $(CFLAGS:__KERNEL__=__TASK__) -c -o $@ $<

tasks/%.elf: tasks/pso_head.o tasks/syscalls.o tasks/lib.o tasks/%.o tasks/pso_tail.o $(BIN_LIB)
	$(LD) $(LDFLAGS) -N -e main -Ttext 0x400000 -o $@ tasks/pso_head.o tasks/syscalls.o tasks/lib.o $(@:.elf=.o) $(LDPSO) tasks/pso_tail.o

$(BIN_KERN).orig.elf: $(OBJS_KERN) $(SYMBOLS)_null.o
	$(LD) $(LDFLAGS) -N -e start -Ttext 0x1200 -o $@ $(OBJS_KERN) $(SYMBOLS)_null.o $(LDPSO)

$(BIN_KERN).elf: $(BIN_KERN).orig.elf $(SYMBOLS).o
	$(LD) $(LDFLAGS) -N -e start -Ttext 0x1200 -o $@ $(OBJS_KERN) $(SYMBOLS).o $(LDPSO)

$(BIN_KERN): $(BIN_KERN).elf
	$(OBJCOPY) -S -O binary $< $@

$(BIN_KERN).asm: $(BIN_KERN).elf
	$(OBJDUMP) -S -M intel $< >$@

$(BIN_KERN).dbg: $(BIN_KERN).elf
	$(OBJCOPY) --only-keep-debug $< $@

$(SYM_KERN): $(BIN_KERN).orig.elf
	nm $< | cut -d ' ' -f 1,3 > $@

$(SYMBOLS).asm: $(SYM_KERN)
	cat $(SYM_KERN) | python gensym.py > $(SYMBOLS).asm

$(SYMBOLS).o: $(SYMBOLS).asm
	nasm $(NASMFLAGS) -felf32 -o $@ $<

$(SYMBOLS)_null.asm:
	python gensym.py --null > $(SYMBOLS)_null.asm

$(SYMBOLS)_null.o: $(SYMBOLS)_null.asm
	nasm $(NASMFLAGS) -felf32 -o $@ $<

# Images
$(IMG_BASE):
	@dd if=/dev/zero of=$@ bs=512 count=2880 2>/dev/null
	$(MKFS) -F 12 -f 2 -n $(DISK_LABEL) $@

$(IMG_FLOPPY): $(BIN_BOOT) $(BIN_KERN) $(IMG_BASE)
	[ -f $(IMG_FLOPPY) ] || dd if=$(IMG_BASE) of=$@ bs=512 count=2880 2>/dev/null
	dd if=$< of=$@ bs=1 count=3 conv=notrunc 2>/dev/null
	dd if=$< of=$@ bs=1 count=450 seek=62 skip=62 conv=notrunc 2>/dev/null
	mcopy -obi $@ $(BIN_KERN) ::kernel.bin
	#for T in $(TASKS); do mcopy -obi $@ $$T ::`basename $$T`; done;

$(IMG_HDD): $(TASKS) $(TESTS) $(BIN_KERN)
	rm -f $(IMG_HDD)
	bximage -q -hd -mode=flat -size=10 $@ >/dev/null
	mkfs.ext2 -F -L SpuriOS $@ > /dev/null
	sudo mkdir -p /tmp/spurios-hdd
	sudo mount -o loop bin/hdd.img /tmp/spurios-hdd
	sudo mkdir -p /tmp/spurios-hdd/bin
	sudo mkdir -p /tmp/spurios-hdd/tests
	sudo mkdir -p /tmp/spurios-hdd/doc
	sudo cp README.md /tmp/spurios-hdd/
	sudo cp doc/*.md /tmp/spurios-hdd/doc/
	sudo cp $(BIN_KERN) /tmp/spurios-hdd/
	for T in $(TASKS); do sudo cp -f $$T /tmp/spurios-hdd/bin; done;
	for T in $(TESTS); do sudo cp -f $$T /tmp/spurios-hdd/tests; done;
	sudo umount /tmp/spurios-hdd

# Documentation
doc: $(DOC_FILES)

doc/%.pdf: doc/%.md
	@markdown2pdf -o $@ $<

# Otros
clean:
	rm -f $(IMG_HDD) $(BIN_KERN) $(IMG_FLOPPY) $(IMG_BASE) $(OBJS_KERN) $(DUMP_KERN) $(SYM_KERN) $(SYMBOLS_FILES) $(OBJS_TASKS) $(TASKS) $(TASKS:.pso=.elf) $(OBJS_TESTS) $(TESTS) $(TESTS:.pso=.elf)
	#rm -f deps

# Dependencias
deps: Makefile
	makedepend -I include/ kernel/*.c tasks/*.c $(OBJS_LIB:.o=.c) -f- > deps
	for f in kernel/*.asm; do nasm $(NASMFLAGS) -felf32 -I $(DIRKERN) -M $$f; done >> deps 2>/dev/null
#	$(CC) $(CFLAGS) -MM kernel/*.c $(OBJS_LIB:.o=.c) > deps

-include deps

# DO NOT DELETE
