#pragma once
#include <stdint.h>

void keyboard_init(void);
char keyboard_getchar(void);
int  keyboard_haschar(void);

static inline void outb(uint16_t port, uint8_t val) {
    __asm__ volatile("outb %0, %1" : : "a"(val), "Nd"(port));
}
