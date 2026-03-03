#include "../include/vga.h"
#include "../include/gdt.h"
#include "../include/idt.h"
#include "../include/keyboard.h"
#include "../include/shell.h"
#include <stdint.h>

void kernel_main(uint32_t magic, uint64_t mbi) {
    (void)magic; (void)mbi;
    vga_init();
    vga_set_color(VGA_LIGHT_GREEN, VGA_BLACK);
    vga_puts("KOKIRI x86_64 Kernel\n");
    vga_set_color(VGA_WHITE, VGA_BLACK);
    gdt_init();
    idt_init();
    keyboard_init();
    __asm__("sti");
    shell_run();
}
