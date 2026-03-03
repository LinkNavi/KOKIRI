#include "../include/gdt.h"
#include "../include/vga.h"
#include <stdint.h>

typedef struct __attribute__((packed)) {
    uint16_t limit_low;
    uint16_t base_low;
    uint8_t  base_mid;
    uint8_t  access;
    uint8_t  granularity;
    uint8_t  base_high;
} gdt_entry_t;

typedef struct __attribute__((packed)) {
    uint16_t limit;
    uint64_t base;
} gdt_ptr_t;

static gdt_entry_t gdt[5];
static gdt_ptr_t   gdt_ptr;

extern void gdt_flush(uint64_t);

static void gdt_set(int i, uint8_t access, uint8_t gran) {
    gdt[i].base_low = gdt[i].base_mid = gdt[i].base_high = 0;
    gdt[i].limit_low = 0;
    gdt[i].access = access;
    gdt[i].granularity = gran;
}

void gdt_init(void) {
    gdt_ptr.limit = sizeof(gdt) - 1;
    gdt_ptr.base  = (uint64_t)&gdt;
    gdt_set(0, 0x00, 0x00);
    gdt_set(1, 0x9A, 0x20);
    gdt_set(2, 0x92, 0x00);
    gdt_set(3, 0xFA, 0x20);
    gdt_set(4, 0xF2, 0x00);
    gdt_flush((uint64_t)&gdt_ptr);
    vga_puts("[GDT] Initialized\n");
}
