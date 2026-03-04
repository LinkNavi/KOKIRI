#include "../include/vga.h"
#include "../include/gdt.h"
#include "../include/idt.h"
#include "../include/keyboard.h"
#include "../include/shell.h"
#include "../include/pmm.h"
#include "../include/vmm.h"
#include "../include/heap.h"
#include <stdint.h>

static void vga_putuint(uint64_t n) {
    if (n == 0) { vga_putchar('0'); return; }
    char buf[21]; int i = 0;
    while (n) { buf[i++] = '0' + (n % 10); n /= 10; }
    for (int j = i-1; j >= 0; j--) vga_putchar(buf[j]);
}

void kernel_main(uint32_t magic, uint64_t mbi) {
    (void)magic;
    vga_init();
    vga_set_color(VGA_LIGHT_GREEN, VGA_BLACK);
    vga_puts("KOKIRI x86_64 Kernel\n");
    vga_set_color(VGA_WHITE, VGA_BLACK);
    gdt_init();
    idt_init();
    pmm_init(mbi);
    vga_puts("[PMM] Free: ");
    vga_putuint(pmm_free_pages() * 4 / 1024);
    vga_puts(" MB\n");
    vmm_init();
    heap_init();
    keyboard_init();
    __asm__("sti");
    shell_run();
}
