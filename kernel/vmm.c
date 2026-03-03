#include "../include/vmm.h"
#include "../include/pmm.h"
#include "../include/vga.h"
#include <stddef.h>

// get index into each level of the page table from a virtual address
#define PML4_IDX(v) (((v) >> 39) & 0x1FF)
#define PDP_IDX(v)  (((v) >> 30) & 0x1FF)
#define PD_IDX(v)   (((v) >> 21) & 0x1FF)
#define PT_IDX(v)   (((v) >> 12) & 0x1FF)
#define PAGE_MASK   (~(uint64_t)0xFFF)

static pagemap_t kernel_pagemap = 0;

static uint64_t *get_or_create(uint64_t *table, uint64_t idx, uint64_t flags) {
    if (!(table[idx] & PAGE_PRESENT)) {
        void *page = pmm_alloc();
        if (!page) return 0;
        // zero the new table
        uint64_t *p = (uint64_t *)(uintptr_t)page;
        for (int i = 0; i < 512; i++) p[i] = 0;
        table[idx] = (uint64_t)(uintptr_t)page | flags;
    }
    return (uint64_t *)(uintptr_t)(table[idx] & PAGE_MASK);
}

void vmm_map(pagemap_t pm, uint64_t virt, uint64_t phys, uint64_t flags) {
    uint64_t *pdp = get_or_create(pm,  PML4_IDX(virt), PAGE_PRESENT | PAGE_WRITE | (flags & PAGE_USER));
    if (!pdp) return;
    uint64_t *pd  = get_or_create(pdp, PDP_IDX(virt),  PAGE_PRESENT | PAGE_WRITE | (flags & PAGE_USER));
    if (!pd)  return;
    uint64_t *pt  = get_or_create(pd,  PD_IDX(virt),   PAGE_PRESENT | PAGE_WRITE | (flags & PAGE_USER));
    if (!pt)  return;
    pt[PT_IDX(virt)] = (phys & PAGE_MASK) | flags | PAGE_PRESENT;
}

void vmm_unmap(pagemap_t pm, uint64_t virt) {
    uint64_t *pdp = (uint64_t *)(uintptr_t)(pm[PML4_IDX(virt)] & PAGE_MASK);
    if (!pdp) return;
    uint64_t *pd  = (uint64_t *)(uintptr_t)(pdp[PDP_IDX(virt)] & PAGE_MASK);
    if (!pd)  return;
    uint64_t *pt  = (uint64_t *)(uintptr_t)(pd[PD_IDX(virt)] & PAGE_MASK);
    if (!pt)  return;
    pt[PT_IDX(virt)] = 0;
    // flush TLB for this page
    __asm__ volatile("invlpg (%0)" :: "r"(virt) : "memory");
}

void vmm_switch(pagemap_t pm) {
    __asm__ volatile("mov %0, %%cr3" :: "r"((uint64_t)(uintptr_t)pm) : "memory");
}

uint64_t vmm_virt_to_phys(pagemap_t pm, uint64_t virt) {
    uint64_t *pdp = (uint64_t *)(uintptr_t)(pm[PML4_IDX(virt)] & PAGE_MASK);
    if (!pdp) return 0;
    uint64_t *pd  = (uint64_t *)(uintptr_t)(pdp[PDP_IDX(virt)] & PAGE_MASK);
    if (!pd)  return 0;
    uint64_t *pt  = (uint64_t *)(uintptr_t)(pd[PD_IDX(virt)] & PAGE_MASK);
    if (!pt)  return 0;
    uint64_t entry = pt[PT_IDX(virt)];
    if (!(entry & PAGE_PRESENT)) return 0;
    return (entry & PAGE_MASK) | (virt & 0xFFF);
}

pagemap_t vmm_new_pagemap(void) {
    pagemap_t pm = (pagemap_t)(uintptr_t)pmm_alloc();
    if (!pm) return 0;
    for (int i = 0; i < 512; i++) pm[i] = 0;
    // copy kernel mappings (upper half) from kernel pagemap
    if (kernel_pagemap) {
        for (int i = 256; i < 512; i++)
            pm[i] = kernel_pagemap[i];
    }
    return pm;
}

void vmm_init(void) {
    pagemap_t pm = (pagemap_t)(uintptr_t)pmm_alloc();
    if (!pm) { vga_puts("[VMM] Out of memory!\n"); return; }
    for (int i = 0; i < 512; i++) pm[i] = 0;

    // identity map first 4GB for kernel
    for (uint64_t addr = 0; addr < 0x100000000ULL; addr += PAGE_SIZE) {
        vmm_map(pm, addr, addr, PAGE_PRESENT | PAGE_WRITE);
    }

    kernel_pagemap = pm;
    vmm_switch(pm);
    vga_puts("[VMM] Initialized\n");
}
