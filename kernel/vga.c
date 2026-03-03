#include "../include/vga.h"
#define VGA_WIDTH  80
#define VGA_HEIGHT 25
#define VGA_MEM    ((uint16_t*)0xB8000)
static size_t col, row;
static uint8_t color;
static inline uint16_t entry(char c, uint8_t col) { return (uint16_t)c | ((uint16_t)col << 8); }
void vga_set_color(uint8_t fg, uint8_t bg) { color = fg | (bg << 4); }
void vga_clear(void) {
    for (size_t i = 0; i < VGA_WIDTH*VGA_HEIGHT; i++) VGA_MEM[i] = entry(' ', color);
    col = row = 0;
}
void vga_init(void) { vga_set_color(VGA_WHITE, VGA_BLACK); vga_clear(); }
static void newline(void) { col = 0; if (++row == VGA_HEIGHT) row = 0; }
void vga_putchar(char c) {
    if (c == '\n') { newline(); return; }
    if (c == '\r') { col = 0; return; }
    VGA_MEM[row*VGA_WIDTH + col] = entry(c, color);
    if (++col == VGA_WIDTH) newline();
}
void vga_puts(const char *s) { while (*s) vga_putchar(*s++); }
