#include "../include/pmm.h"
#include "../include/vga.h"

#define MAX_PAGES    (512 * 1024)       // 2GB max
#define BITMAP_SIZE  (MAX_PAGES / 64)

static uint64_t bitmap[BITMAP_SIZE];
static uint64_t bitmap_size = 0;
static uint64_t free_pages  = 0;

// multiboot2 tags
#define MB2_TAG_END  0
#define MB2_TAG_MMAP 6

typedef struct __attribute__((packed)) {
    uint32_t type;
    uint32_t size;
} mb2_tag_t;

typedef struct __attribute__((packed)) {
    uint32_t type;
    uint32_t size;
    uint32_t entry_size;
    uint32_t entry_version;
} mb2_tag_mmap_t;

typedef struct __attribute__((packed)) {
    uint64_t base_addr;
    uint64_t length;
    uint32_t type;
    uint32_t reserved;
} mb2_mmap_entry_t;

static inline void bitmap_set(uint64_t page) {
    bitmap[page / 64] |= (1ULL << (page % 64));
}
static inline void bitmap_clear(uint64_t page) {
    bitmap[page / 64] &= ~(1ULL << (page % 64));
}
static inline int bitmap_test(uint64_t page) {
    return (bitmap[page / 64] >> (page % 64)) & 1;
}

static void mark_used(uint64_t base, uint64_t len) {
    uint64_t start = base / PAGE_SIZE;
    uint64_t end   = (base + len + PAGE_SIZE - 1) / PAGE_SIZE;
    for (uint64_t i = start; i < end && i < bitmap_size; i++) {
        if (!bitmap_test(i)) { bitmap_set(i); free_pages--; }
    }
}

void pmm_init(uint64_t mbi_addr) {
vga_puts("[PMM] entering init\n");
    // start all reserved
    for (uint64_t i = 0; i < BITMAP_SIZE; i++) bitmap[i] = ~0ULL;
    free_pages = 0;

    // walk tags
    uint64_t addr = mbi_addr + 8;
    mb2_tag_mmap_t *mmap_tag = 0;

    for (;;) {
        mb2_tag_t *tag = (mb2_tag_t *)(uintptr_t)addr;
        if (tag->type == MB2_TAG_END || tag->size == 0) break;
        if (tag->type == MB2_TAG_MMAP)
            mmap_tag = (mb2_tag_mmap_t *)(uintptr_t)addr;
        addr = (addr + tag->size + 7) & ~(uint64_t)7;
    }

    if (!mmap_tag) {
        vga_puts("[PMM] No mmap!\n");
        return;
    }

    // find highest address
    uint64_t highest = 0;
    uint64_t eaddr = (uint64_t)(uintptr_t)mmap_tag + sizeof(mb2_tag_mmap_t);
    uint64_t etop  = (uint64_t)(uintptr_t)mmap_tag + mmap_tag->size;

    while (eaddr + mmap_tag->entry_size <= etop) {
        mb2_mmap_entry_t *e = (mb2_mmap_entry_t *)(uintptr_t)eaddr;
        if (e->type == 1) {
            uint64_t top = e->base_addr + e->length;
            if (top > highest) highest = top;
        }
        eaddr += mmap_tag->entry_size;
    }

    bitmap_size = highest / PAGE_SIZE;
    if (bitmap_size > MAX_PAGES) bitmap_size = MAX_PAGES;

    // free available regions
    eaddr = (uint64_t)(uintptr_t)mmap_tag + sizeof(mb2_tag_mmap_t);
    while (eaddr + mmap_tag->entry_size <= etop) {
        mb2_mmap_entry_t *e = (mb2_mmap_entry_t *)(uintptr_t)eaddr;
        if (e->type == 1) {
            uint64_t start = e->base_addr / PAGE_SIZE;
            uint64_t end   = (e->base_addr + e->length) / PAGE_SIZE;
            for (uint64_t i = start; i < end && i < bitmap_size; i++) {
                bitmap_clear(i);
                free_pages++;
            }
        }
        eaddr += mmap_tag->entry_size;
    }

    // reserve low memory + null page
    mark_used(0, 0x100000);

    // reserve kernel: 1MB to 4MB to be safe
    mark_used(0x100000, 0x400000 - 0x100000);

    // reserve mbi itself
    mark_used(mbi_addr, 4096);

    vga_puts("[PMM] Initialized\n");
}

void *pmm_alloc(void) {
    for (uint64_t i = 0; i < bitmap_size / 64; i++) {
        if (bitmap[i] == ~0ULL) continue;
        for (int b = 0; b < 64; b++) {
            uint64_t page = i * 64 + b;
            if (page >= bitmap_size) return 0;
            if (!bitmap_test(page)) {
                bitmap_set(page);
                free_pages--;
                return (void *)(uintptr_t)(page * PAGE_SIZE);
            }
        }
    }
    return 0;
}

void pmm_free(void *ptr) {
    uint64_t page = (uint64_t)(uintptr_t)ptr / PAGE_SIZE;
    if (page >= bitmap_size) return;
    if (bitmap_test(page)) { bitmap_clear(page); free_pages++; }
}

uint64_t pmm_free_pages(void) { return free_pages; }
