ifeq ($(OS),Windows_NT)
	CC      = clang
	LD      = ld.lld
	CFLAGS  = --target=x86_64-elf -ffreestanding -fno-stack-protector -fno-pic -mno-red-zone -mcmodel=kernel -O2 -Wall -Wextra -Iinclude
	LDFLAGS = -m elf_x86_64 --script linker.ld
else
	CC      = gcc
	LD      = ld
CFLAGS = -ffreestanding -fno-pic -mno-red-zone -mcmodel=small -mno-sse -mno-sse2 -mno-mmx -O2 -Wall -Wextra -Iinclude
LDFLAGS = -m elf_x86_64 -T linker.ld
endif

ASFLAGS = -f elf64 -w-other
OBJS    = boot/boot.o boot/gdt_flush.o boot/idt_flush.o boot/isr.o \
          kernel/vga.o kernel/gdt.o kernel/idt.o kernel/keyboard.o \
          kernel/shell.o kernel/pmm.o kernel/kernel.o

kernel.bin: $(OBJS)
	$(LD) $(LDFLAGS) -o $@ $^

%.o: %.asm
	nasm $(ASFLAGS) -o $@ $<

%.o: %.c
	$(CC) $(CFLAGS) -c -o $@ $<

run: kernel.bin
	bash run.sh

clean:
	rm -f $(OBJS) kernel.bin kokiri.iso
	rm -rf iso
