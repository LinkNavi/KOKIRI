#pragma once
#include <stdint.h>
#include <stddef.h>

#define PAGE_SIZE 4096

void  pmm_init(uint64_t mbi_addr);
void *pmm_alloc(void);
void  pmm_free(void *page);
uint64_t pmm_free_pages(void);
