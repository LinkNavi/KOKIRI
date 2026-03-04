#include "../include/heap.h"
#include "../include/pmm.h"
#include "../include/vga.h"
#include <stddef.h>
#include <stdint.h>

// simple free-list heap
// each allocation has a header immediately before it

#define HEAP_MAGIC 0xC0CAC01A

typedef struct block {
    uint32_t      magic;
    uint32_t      size;     // usable bytes (not including header)
    uint8_t       free;
    struct block *next;
    struct block *prev;
} block_t;

#define HEADER_SIZE sizeof(block_t)
#define HEAP_PAGES  256                          // 1MB initial heap
#define MIN_SPLIT   (HEADER_SIZE + 16)           // min size to split a block

static block_t *heap_start = 0;
static block_t *heap_end   = 0;

static block_t *expand(size_t min_size) {
    size_t pages = (min_size + HEADER_SIZE + PAGE_SIZE - 1) / PAGE_SIZE;
    if (pages < 4) pages = 4;

    void *mem = pmm_alloc();
    for (size_t i = 1; i < pages; i++) pmm_alloc(); // contiguous alloc
    // note: for a real OS you'd map these virtually — for now pmm gives
    // identity-mapped pages so this works while we're in kernel space

    block_t *b = (block_t *)(uintptr_t)mem;
    b->magic = HEAP_MAGIC;
    b->size  = pages * PAGE_SIZE - HEADER_SIZE;
    b->free  = 1;
    b->next  = 0;
    b->prev  = heap_end;

    if (heap_end) heap_end->next = b;
    heap_end = b;
    if (!heap_start) heap_start = b;

    return b;
}

void heap_init(void) {
    // allocate initial heap pages
    void *mem = 0;
    for (int i = 0; i < HEAP_PAGES; i++) {
        void *p = pmm_alloc();
        if (i == 0) mem = p;
    }

    heap_start = (block_t *)(uintptr_t)mem;
    heap_start->magic = HEAP_MAGIC;
    heap_start->size  = HEAP_PAGES * PAGE_SIZE - HEADER_SIZE;
    heap_start->free  = 1;
    heap_start->next  = 0;
    heap_start->prev  = 0;
    heap_end = heap_start;

    vga_puts("[HEAP] Initialized\n");
}

void *kmalloc(size_t size) {
    if (!size) return 0;
    // align to 16 bytes
    size = (size + 15) & ~(size_t)15;

    block_t *b = heap_start;
    while (b) {
        if (b->magic != HEAP_MAGIC) {
            vga_puts("[HEAP] Corruption detected!\n");
            return 0;
        }
        if (b->free && b->size >= size) {
            // split if large enough
            if (b->size >= size + MIN_SPLIT) {
                block_t *split = (block_t *)((uint8_t *)b + HEADER_SIZE + size);
                split->magic = HEAP_MAGIC;
                split->size  = b->size - size - HEADER_SIZE;
                split->free  = 1;
                split->next  = b->next;
                split->prev  = b;
                if (b->next) b->next->prev = split;
                else heap_end = split;
                b->next = split;
                b->size = size;
            }
            b->free = 0;
            return (void *)((uint8_t *)b + HEADER_SIZE);
        }
        if (!b->next) b = expand(size);
        else b = b->next;
    }
    return 0;
}

void *kcalloc(size_t count, size_t size) {
    size_t total = count * size;
    void *ptr = kmalloc(total);
    if (ptr) {
        uint8_t *p = (uint8_t *)ptr;
        for (size_t i = 0; i < total; i++) p[i] = 0;
    }
    return ptr;
}

void *krealloc(void *ptr, size_t size) {
    if (!ptr) return kmalloc(size);
    if (!size) { kfree(ptr); return 0; }

    block_t *b = (block_t *)((uint8_t *)ptr - HEADER_SIZE);
    if (b->magic != HEAP_MAGIC) return 0;
    if (b->size >= size) return ptr;

    void *new = kmalloc(size);
    if (!new) return 0;
    uint8_t *src = (uint8_t *)ptr;
    uint8_t *dst = (uint8_t *)new;
    size_t copy = b->size < size ? b->size : size;
    for (size_t i = 0; i < copy; i++) dst[i] = src[i];
    kfree(ptr);
    return new;
}

void kfree(void *ptr) {
    if (!ptr) return;
    block_t *b = (block_t *)((uint8_t *)ptr - HEADER_SIZE);
    if (b->magic != HEAP_MAGIC) {
        vga_puts("[HEAP] kfree: bad pointer\n");
        return;
    }
    b->free = 1;

    // coalesce with next
    if (b->next && b->next->free) {
        b->size += HEADER_SIZE + b->next->size;
        b->next  = b->next->next;
        if (b->next) b->next->prev = b;
        else heap_end = b;
    }
    // coalesce with prev
    if (b->prev && b->prev->free) {
        b->prev->size += HEADER_SIZE + b->size;
        b->prev->next  = b->next;
        if (b->next) b->next->prev = b->prev;
        else heap_end = b->prev;
    }
}
