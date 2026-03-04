#include "../include/process.h"
#include "../include/pmm.h"
#include "../include/vmm.h"
#include "../include/tss.h"
#include "../include/vga.h"
#include "../include/string.h"

static process_t procs[PROC_MAX];
static int current_pid = -1;

extern void enter_usermode(uint64_t rip, uint64_t rsp, uint64_t rflags);
extern void syscall_set_kernel_stack(uint64_t rsp0);

void proc_init(void) {
    memset(procs, 0, sizeof(procs));
    for (int i = 0; i < PROC_MAX; i++) {
        procs[i].state = PROC_UNUSED;
        procs[i].pid = i;
    }
    vga_puts("[PROC] Initialized\n");
}

process_t *proc_current(void) {
    if (current_pid < 0 || current_pid >= PROC_MAX) return 0;
    return &procs[current_pid];
}

process_t *proc_create(void) {
    for (int i = 0; i < PROC_MAX; i++) {
        if (procs[i].state == PROC_UNUSED) {
            procs[i].state = PROC_READY;

            // Create a new page map, copying kernel mappings
            procs[i].pagemap = vmm_new_pagemap();
            if (!procs[i].pagemap) {
                vga_puts("[PROC] No memory for pagemap\n");
                return 0;
            }

            // Allocate kernel stack for this process (for syscall/interrupt handling)
            void *kstack = pmm_alloc();
            if (!kstack) return 0;
            // Allocate a few more pages for the kernel stack
            for (int j = 0; j < 3; j++) pmm_alloc();
            procs[i].kernel_stack = (uint64_t)(uintptr_t)kstack + 4 * PAGE_SIZE;

            // Allocate user stack
            uint64_t stack_base = USER_STACK_TOP - USER_STACK_SIZE;
            for (uint64_t a = stack_base; a < USER_STACK_TOP; a += PAGE_SIZE) {
                void *phys = pmm_alloc();
                if (!phys) return 0;
                memset((void *)(uintptr_t)phys, 0, PAGE_SIZE);
                vmm_map(procs[i].pagemap, a, (uint64_t)(uintptr_t)phys,
                        PAGE_PRESENT | PAGE_WRITE | PAGE_USER);
            }

            procs[i].ctx.rsp = USER_STACK_TOP - 8; // leave room
            procs[i].ctx.rflags = 0x202; // IF set

            return &procs[i];
        }
    }
    return 0;
}

void proc_enter_usermode(process_t *p) {
    current_pid = p->pid;
    p->state = PROC_RUNNING;

    // Set kernel stack for syscalls/interrupts
    syscall_set_kernel_stack(p->kernel_stack);

    // Switch to process page map
    vmm_switch(p->pagemap);

    // Jump to user mode
    enter_usermode(p->ctx.rip, p->ctx.rsp, p->ctx.rflags);
}
