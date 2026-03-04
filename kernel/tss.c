#include "../include/tss.h"
#include "../include/vga.h"
#include "../include/string.h"

// The TSS needs to be installed into the GDT as a system segment (16 bytes)
// We'll modify gdt.c to add it, but for now expose the TSS and a setup function

static tss_t tss;
static uint8_t tss_stack[8192] __attribute__((aligned(16)));

extern void gdt_load_tss(uint64_t tss_addr, uint16_t tss_size);

tss_t *tss_get(void) { return &tss; }

void tss_init(void) {
    memset(&tss, 0, sizeof(tss));
    tss.rsp0 = (uint64_t)&tss_stack[sizeof(tss_stack)];
    tss.iopb_offset = sizeof(tss);
    vga_puts("[TSS] Initialized\n");
}

void tss_set_rsp0(uint64_t rsp0) {
    tss.rsp0 = rsp0;
}
