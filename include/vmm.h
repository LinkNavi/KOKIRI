#pragma once
#include <stdint.h>
#include <stddef.h>

#define PAGE_PRESENT  (1ULL << 0)
#define PAGE_WRITE    (1ULL << 1)
#define PAGE_USER     (1ULL << 2)
#define PAGE_HUGE     (1ULL << 7)
#define PAGE_NX       (1ULL << 63)

typedef uint64_t* pagemap_t;

void      vmm_init(void);
pagemap_t vmm_new_pagemap(void);
void      vmm_map(pagemap_t pm, uint64_t virt, uint64_t phys, uint64_t flags);
void      vmm_unmap(pagemap_t pm, uint64_t virt);
void      vmm_switch(pagemap_t pm);
uint64_t  vmm_virt_to_phys(pagemap_t pm, uint64_t virt);
