#!/bin/bash
set -e
echo "[KOKIRI] Building x86_64..."

ASFLAGS="-f elf64"
OBJS="boot/boot.o boot/gdt_flush.o boot/idt_flush.o boot/isr.o kernel/vga.o kernel/gdt.o kernel/idt.o kernel/keyboard.o kernel/shell.o kernel/pmm.o kernel/kernel.o"

nasm $ASFLAGS boot/boot.asm      -o boot/boot.o
nasm $ASFLAGS boot/gdt_flush.asm -o boot/gdt_flush.o
nasm $ASFLAGS boot/idt_flush.asm -o boot/idt_flush.o
nasm $ASFLAGS boot/isr.asm       -o boot/isr.o

if [ "$OS" = "Windows_NT" ]; then
    CF="--target=x86_64-elf -ffreestanding -fno-pic -fno-stack-protector -mno-red-zone -mcmodel=kernel -O2 -Iinclude"
    clang $CF -c kernel/vga.c      -o kernel/vga.o
    clang $CF -c kernel/gdt.c      -o kernel/gdt.o
    clang $CF -c kernel/idt.c      -o kernel/idt.o
    clang $CF -c kernel/keyboard.c -o kernel/keyboard.o
    clang $CF -c kernel/shell.c    -o kernel/shell.o
    clang $CF -c kernel/pmm.c      -o kernel/pmm.o
    clang $CF -c kernel/kernel.c   -o kernel/kernel.o
    ld.lld -m elf_x86_64 --script linker.ld -o kernel.bin $OBJS
else
    CF="-ffreestanding -fno-pic -mno-red-zone -mcmodel=kernel -fno-stack-protector -O2 -Iinclude"
    gcc $CF -c kernel/vga.c      -o kernel/vga.o
    gcc $CF -c kernel/gdt.c      -o kernel/gdt.o
    gcc $CF -c kernel/idt.c      -o kernel/idt.o
    gcc $CF -c kernel/keyboard.c -o kernel/keyboard.o
    gcc $CF -c kernel/shell.c    -o kernel/shell.o
    gcc $CF -c kernel/pmm.c      -o kernel/pmm.o
    gcc $CF -c kernel/kernel.c   -o kernel/kernel.o
    ld -m elf_x86_64 -T linker.ld -o kernel.bin $OBJS
fi

echo "[KOKIRI] Done! $(du -h kernel.bin | cut -f1)"
