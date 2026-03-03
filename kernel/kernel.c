#include "../include/vga.h"
#include "../include/gdt.h"
#include "../include/idt.h"
#include "../include/keyboard.h"
#include "../include/shell.h"
#include "../include/pmm.h"
#include <stdint.h>

static void vga_putuint(uint64_t n) {
    if (n == 0) { vga_putchar('0'); return; }
    char buf[21]; int i = 0;
    while (n) { buf[i++] = '0' + (n % 10); n /= 10; }
    for (int j = i-1; j >= 0; j--) vga_putchar(buf[j]);
}

void kernel_main(uint32_t magic, uint64_t mbi) {
    vga_init();
    vga_set_color(VGA_LIGHT_GREEN, VGA_BLACK);
    vga_puts("KOKIRI x86_64 Kernel\n");
    vga_set_color(VGA_WHITE, VGA_BLACK);

    // Debug: print magic and mbi
    vga_puts("magic: 0x");
    // print hex of magic
    for (int i = 28; i >= 0; i -= 4) {
        char c = "0123456789ABCDEF"[(magic >> i) & 0xF];
        vga_putchar(c);
    }
    vga_puts("\nmbi: 0x");
    for (int i = 60; i >= 0; i -= 4) {
        char c = "0123456789ABCDEF"[(mbi >> i) & 0xF];
        vga_putchar(c);
    }
    vga_puts("\n");

    gdt_init();
    idt_init();
    pmm_init(mbi);  // leave commented for now
    keyboard_init();
    __asm__("sti");
    shell_run();
}
