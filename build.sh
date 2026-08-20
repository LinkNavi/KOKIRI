#!/bin/bash
set -e
echo "[KOKIRI] Building x86_64..."

ASFLAGS="-f elf64 -w-other"
OBJS="boot/boot.o boot/gdt_flush.o boot/idt_flush.o boot/isr.o boot/syscall_entry.o boot/usermode.o kernel/vga.o kernel/gdt.o kernel/idt.o kernel/keyboard.o kernel/shell.o kernel/pmm.o kernel/vmm.o kernel/heap.o kernel/string.o kernel/tss.o kernel/syscall.o kernel/vfs.o kernel/process.o kernel/elf.o kernel/kernel.o"

nasm $ASFLAGS boot/boot.asm      -o boot/boot.o
nasm $ASFLAGS boot/gdt_flush.asm -o boot/gdt_flush.o
nasm $ASFLAGS boot/idt_flush.asm -o boot/idt_flush.o
nasm $ASFLAGS boot/isr.asm       -o boot/isr.o
nasm $ASFLAGS boot/syscall_entry.asm -o boot/syscall_entry.o
nasm $ASFLAGS boot/usermode.asm  -o boot/usermode.o

if [ "$OS" = "Windows_NT" ]; then
    CF="--target=x86_64-elf -ffreestanding -fno-pic -fno-stack-protector -mno-red-zone -mcmodel=kernel -O2 -Iinclude"
    clang $CF -c kernel/vga.c      -o kernel/vga.o
    clang $CF -c kernel/gdt.c      -o kernel/gdt.o
    clang $CF -c kernel/idt.c      -o kernel/idt.o
    clang $CF -c kernel/keyboard.c -o kernel/keyboard.o
    clang $CF -c kernel/shell.c    -o kernel/shell.o
    clang $CF -c kernel/pmm.c      -o kernel/pmm.o
    clang $CF -c kernel/vmm.c      -o kernel/vmm.o
    clang $CF -c kernel/heap.c     -o kernel/heap.o
    clang $CF -c kernel/string.c   -o kernel/string.o
    clang $CF -c kernel/tss.c      -o kernel/tss.o
    clang $CF -c kernel/syscall.c  -o kernel/syscall.o
    clang $CF -c kernel/vfs.c      -o kernel/vfs.o
    clang $CF -c kernel/process.c  -o kernel/process.o
    clang $CF -c kernel/elf.c      -o kernel/elf.o
    clang $CF -c kernel/kernel.c   -o kernel/kernel.o
    ld.lld -m elf_x86_64 --script linker.ld -o kernel.bin $OBJS
else
    CF="-ffreestanding -fno-pic -fno-stack-protector -mno-red-zone -mcmodel=small -mno-sse -mno-sse2 -mno-mmx -O2 -Iinclude"
    gcc $CF -c kernel/vga.c      -o kernel/vga.o
    gcc $CF -c kernel/gdt.c      -o kernel/gdt.o
    gcc $CF -c kernel/idt.c      -o kernel/idt.o
    gcc $CF -c kernel/keyboard.c -o kernel/keyboard.o
    gcc $CF -c kernel/shell.c    -o kernel/shell.o
    gcc $CF -c kernel/pmm.c      -o kernel/pmm.o
    gcc $CF -c kernel/vmm.c      -o kernel/vmm.o
    gcc $CF -c kernel/heap.c     -o kernel/heap.o
    gcc $CF -c kernel/string.c   -o kernel/string.o
    gcc $CF -c kernel/tss.c      -o kernel/tss.o
    gcc $CF -c kernel/syscall.c  -o kernel/syscall.o
    gcc $CF -c kernel/vfs.c      -o kernel/vfs.o
    gcc $CF -c kernel/process.c  -o kernel/process.o
    gcc $CF -c kernel/elf.c      -o kernel/elf.o
    gcc $CF -c kernel/kernel.c   -o kernel/kernel.o
    ld -m elf_x86_64 -T linker.ld -o kernel.bin $OBJS
fi

echo "[KOKIRI] Done! $(du -h kernel.bin | cut -f1)"
