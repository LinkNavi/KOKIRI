#include "../include/gdt.h"
#include "../include/tss.h"
#include "../include/vga.h"
#include "../include/string.h"
#include <stdint.h>

// GDT layout:
// 0x00: null
// 0x08: kernel code 64-bit
// 0x10: kernel data
// 0x18: user data  (must be before user code for syscall/sysret)
// 0x20: user code 64-bit
// 0x28: TSS (16 bytes, occupies slots 5-6)

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

// 7 entries: null, kcode, kdata, udata, ucode, tss_lo, tss_hi
static gdt_entry_t gdt[7];
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

    gdt_set(0, 0x00, 0x00);  // null
    gdt_set(1, 0x9A, 0x20);  // 0x08: kernel code (64-bit)
    gdt_set(2, 0x92, 0x00);  // 0x10: kernel data
    gdt_set(3, 0xF2, 0x00);  // 0x18: user data   (DPL=3)
    gdt_set(4, 0xFA, 0x20);  // 0x20: user code   (DPL=3, 64-bit)

    // TSS descriptor will be installed after tss_init
    memset(&gdt[5], 0, sizeof(gdt_entry_t) * 2);

    gdt_flush((uint64_t)&gdt_ptr);
    vga_puts("[GDT] Initialized\n");
}

// Install TSS into GDT slot 5 (0x28) and load TR
void gdt_install_tss(void) {
    extern tss_t *tss_get(void);
    tss_t *tss = tss_get();
    uint64_t base = (uint64_t)tss;
    uint16_t limit = sizeof(tss_t) - 1;

    // Low 8 bytes of TSS descriptor
    gdt[5].limit_low   = limit & 0xFFFF;
    gdt[5].base_low    = base & 0xFFFF;
    gdt[5].base_mid    = (base >> 16) & 0xFF;
    gdt[5].access      = 0x89;  // present, type=9 (available 64-bit TSS)
    gdt[5].granularity = (limit >> 16) & 0x0F;
    gdt[5].base_high   = (base >> 24) & 0xFF;

    // High 8 bytes: upper 32 bits of base + reserved
    uint32_t *tss_high = (uint32_t *)&gdt[6];
    tss_high[0] = (base >> 32) & 0xFFFFFFFF;
    tss_high[1] = 0;

    // Load task register
    __asm__ volatile("ltr %%ax" :: "a"(0x28));
    vga_puts("[GDT] TSS loaded\n");
}
