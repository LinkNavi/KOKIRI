#pragma once
#include <stdint.h>
#include "vmm.h"

#define PROC_MAX       64
#define PROC_RUNNING   1
#define PROC_READY     2
#define PROC_ZOMBIE    3
#define PROC_UNUSED    0

#define USER_STACK_TOP   0x00007FFFFFFFE000ULL
#define USER_STACK_SIZE  (64 * 4096)  // 256KB
#define USER_BRK_START   0x0000000010000000ULL

typedef struct {
    uint64_t rax, rbx, rcx, rdx, rsi, rdi, rbp;
    uint64_t r8, r9, r10, r11, r12, r13, r14, r15;
    uint64_t rip, rsp, rflags;
    uint64_t fs_base;
} context_t;

typedef struct {
    int        pid;
    int        state;
    pagemap_t  pagemap;
    context_t  ctx;
    uint64_t   brk;         // current program break
    uint64_t   brk_start;   // initial brk
    uint64_t   kernel_stack; // rsp0 for TSS
    int        exit_code;
} process_t;

void      proc_init(void);
process_t *proc_create(void);
process_t *proc_current(void);
void       proc_enter_usermode(process_t *p);
