#include "../include/syscall.h"
#include "../include/process.h"
#include "../include/vfs.h"
#include "../include/vga.h"
#include "../include/pmm.h"
#include "../include/vmm.h"
#include "../include/string.h"
#include "../include/tss.h"
#include <stdint.h>
#include <stddef.h>

// MSR addresses for syscall/sysret
#define MSR_EFER    0xC0000080
#define MSR_STAR    0xC0000081
#define MSR_LSTAR   0xC0000082
#define MSR_SFMASK  0xC0000084
#define MSR_FS_BASE 0xC0000100

#define EFER_SCE    (1 << 0)

// kernel rsp0 for syscall entry asm to reference
uint64_t tss_rsp0_ptr = 0;

static inline uint64_t rdmsr(uint32_t msr) {
    uint32_t lo, hi;
    __asm__ volatile("rdmsr" : "=a"(lo), "=d"(hi) : "c"(msr));
    return ((uint64_t)hi << 32) | lo;
}

static inline void wrmsr(uint32_t msr, uint64_t val) {
    __asm__ volatile("wrmsr" :: "a"((uint32_t)val), "d"((uint32_t)(val >> 32)), "c"(msr));
}

extern void syscall_entry(void);

void syscall_init(void) {
    // Enable SCE (syscall enable) in EFER
    uint64_t efer = rdmsr(MSR_EFER);
    efer |= EFER_SCE;
    wrmsr(MSR_EFER, efer);

    // STAR: bits 47:32 = kernel CS (0x08), bits 63:48 = user CS base (0x18)
    // sysret uses (STAR[63:48]+16) for CS and (STAR[63:48]+8) for SS
    // So user CS = 0x18+16 = 0x28 but with | 3 => 0x23 ✗
    // Actually: sysretq loads CS = (STAR[63:48]+16) | 3, SS = (STAR[63:48]+8) | 3
    // We want CS=0x20|3=0x23, SS=0x18|3=0x1B
    // So STAR[63:48] should be 0x10 (0x10+16=0x20 for CS, 0x10+8=0x18 for SS)
    // STAR[47:32] = 0x08 for kernel CS (kernel SS = 0x08+8 = 0x10)
    uint64_t star = ((uint64_t)0x0010 << 48) | ((uint64_t)0x0008 << 32);
    wrmsr(MSR_STAR, star);

    // LSTAR: syscall entry point
    wrmsr(MSR_LSTAR, (uint64_t)syscall_entry);

    // SFMASK: clear IF on syscall entry (bit 9)
    wrmsr(MSR_SFMASK, (1 << 9));

    vga_puts("[SYSCALL] Initialized\n");
}

void syscall_set_kernel_stack(uint64_t rsp0) {
    tss_rsp0_ptr = rsp0;
    tss_set_rsp0(rsp0);
}

// Helper to print hex for debug
static void vga_puthex(uint64_t n) {
    const char *hex = "0123456789ABCDEF";
    vga_puts("0x");
    for (int i = 60; i >= 0; i -= 4) {
        vga_putchar(hex[(n >> i) & 0xF]);
    }
}

// --- Syscall implementations ---

static int64_t sys_write(uint64_t fd, uint64_t buf, uint64_t count) {
    return vfs_write((int)fd, (const void *)buf, (size_t)count);
}

static int64_t sys_read(uint64_t fd, uint64_t buf, uint64_t count) {
    return vfs_read((int)fd, (void *)buf, (size_t)count);
}

static int64_t sys_writev(uint64_t fd, uint64_t iov_ptr, uint64_t iovcnt) {
    return vfs_writev((int)fd, (const iovec_t *)iov_ptr, (int)iovcnt);
}

static int64_t sys_brk(uint64_t addr) {
    process_t *p = proc_current();
    if (!p) return -1;

    if (addr == 0) return (int64_t)p->brk;

    if (addr < p->brk_start) return (int64_t)p->brk;

    // Grow: map new pages
    if (addr > p->brk) {
        uint64_t old_page = (p->brk + PAGE_SIZE - 1) & ~(uint64_t)(PAGE_SIZE - 1);
        uint64_t new_page = (addr + PAGE_SIZE - 1) & ~(uint64_t)(PAGE_SIZE - 1);
        for (uint64_t a = old_page; a < new_page; a += PAGE_SIZE) {
            void *phys = pmm_alloc();
            if (!phys) return (int64_t)p->brk;
            vmm_map(p->pagemap, a, (uint64_t)(uintptr_t)phys, PAGE_PRESENT | PAGE_WRITE | PAGE_USER);
            // zero it
            memset((void *)(uintptr_t)phys, 0, PAGE_SIZE);
        }
    }
    p->brk = addr;
    return (int64_t)p->brk;
}

static int64_t sys_mmap(uint64_t addr, uint64_t length, uint64_t prot,
                         uint64_t flags, uint64_t fd, uint64_t offset) {
    (void)prot; (void)fd; (void)offset;

    // Anonymous mapping only for now
    #define MAP_ANONYMOUS 0x20
    #define MAP_PRIVATE   0x02
    #define MAP_FIXED     0x10

    if (!(flags & MAP_ANONYMOUS)) return -38; // ENOSYS

    process_t *p = proc_current();
    if (!p) return -1;

    // Simple bump allocator for mmap regions
    // Use area above brk
    static uint64_t mmap_base = 0x0000000020000000ULL;

    uint64_t pages = (length + PAGE_SIZE - 1) / PAGE_SIZE;
    uint64_t vaddr;

    if (flags & MAP_FIXED) {
        vaddr = addr & ~(uint64_t)(PAGE_SIZE - 1);
    } else {
        vaddr = mmap_base;
        mmap_base += pages * PAGE_SIZE;
    }

    for (uint64_t i = 0; i < pages; i++) {
        void *phys = pmm_alloc();
        if (!phys) return -12; // ENOMEM
        memset((void *)(uintptr_t)phys, 0, PAGE_SIZE);
        uint64_t pflags = PAGE_PRESENT | PAGE_USER;
        if (prot & 2) pflags |= PAGE_WRITE; // PROT_WRITE
        vmm_map(p->pagemap, vaddr + i * PAGE_SIZE,
                (uint64_t)(uintptr_t)phys, pflags);
    }
    return (int64_t)vaddr;
}

static int64_t sys_munmap(uint64_t addr, uint64_t length) {
    process_t *p = proc_current();
    if (!p) return -1;
    uint64_t pages = (length + PAGE_SIZE - 1) / PAGE_SIZE;
    for (uint64_t i = 0; i < pages; i++) {
        vmm_unmap(p->pagemap, addr + i * PAGE_SIZE);
    }
    return 0;
}

static int64_t sys_mprotect(uint64_t addr, uint64_t len, uint64_t prot) {
    (void)addr; (void)len; (void)prot;
    // Stub — pretend it worked
    return 0;
}

static int64_t sys_exit(uint64_t code) {
    process_t *p = proc_current();
    if (p) {
        p->state = PROC_ZOMBIE;
        p->exit_code = (int)code;
    }
    vga_puts("[PROC] exit(");
    char buf[16]; int i = 0;
    uint64_t n = code;
    if (n == 0) { buf[i++] = '0'; }
    else { while (n) { buf[i++] = '0' + (n % 10); n /= 10; } }
    for (int j = i-1; j >= 0; j--) vga_putchar(buf[j]);
    vga_puts(")\n");

    // Halt — single process for now
    for (;;) __asm__("hlt");
    return 0;
}

static int64_t sys_arch_prctl(uint64_t code, uint64_t addr) {
    #define ARCH_SET_FS 0x1002
    #define ARCH_GET_FS 0x1003
    #define ARCH_SET_GS 0x1001
    #define ARCH_GET_GS 0x1004

    process_t *p = proc_current();

    if (code == ARCH_SET_FS) {
        if (p) p->ctx.fs_base = addr;
        wrmsr(MSR_FS_BASE, addr);
        return 0;
    }
    if (code == ARCH_GET_FS) {
        if (p) *(uint64_t *)addr = p->ctx.fs_base;
        return 0;
    }
    return -22; // EINVAL
}

static int64_t sys_set_tid_address(uint64_t tidptr) {
    (void)tidptr;
    process_t *p = proc_current();
    return p ? p->pid : 1;
}

static int64_t sys_set_robust_list(uint64_t head, uint64_t len) {
    (void)head; (void)len;
    return 0; // stub
}

static int64_t sys_clock_gettime(uint64_t clock_id, uint64_t tp) {
    (void)clock_id;
    // Fake: return 0 time
    if (tp) {
        uint64_t *ts = (uint64_t *)tp;
        ts[0] = 0; // tv_sec
        ts[1] = 0; // tv_nsec
    }
    return 0;
}

static int64_t sys_ioctl(uint64_t fd, uint64_t request, uint64_t arg) {
    (void)fd; (void)request; (void)arg;
    return -25; // ENOTTY
}

static int64_t sys_getpid(void) {
    process_t *p = proc_current();
    return p ? p->pid : 1;
}

static int64_t sys_getrandom(uint64_t buf, uint64_t len, uint64_t flags) {
    (void)flags;
    // Fake random — fill with something
    uint8_t *p = (uint8_t *)buf;
    static uint64_t seed = 12345678;
    for (uint64_t i = 0; i < len; i++) {
        seed = seed * 6364136223846793005ULL + 1442695040888963407ULL;
        p[i] = (uint8_t)(seed >> 33);
    }
    return (int64_t)len;
}

static int64_t sys_fcntl(uint64_t fd, uint64_t cmd, uint64_t arg) {
    (void)fd; (void)cmd; (void)arg;
    return -38; // ENOSYS
}

// Main dispatch
int64_t syscall_dispatch(uint64_t num, uint64_t a1, uint64_t a2,
                          uint64_t a3, uint64_t a4, uint64_t a5) {
    switch (num) {
        case SYS_read:           return sys_read(a1, a2, a3);
        case SYS_write:          return sys_write(a1, a2, a3);
        case SYS_close:          return 0;
        case SYS_fstat:          return -38;
        case SYS_stat:           return -38;
        case SYS_lseek:          return -38;
        case SYS_mmap:           return sys_mmap(a1, a2, a3, a4, a5, 0);
        case SYS_mprotect:       return sys_mprotect(a1, a2, a3);
        case SYS_munmap:         return sys_munmap(a1, a2);
        case SYS_brk:            return sys_brk(a1);
        case SYS_ioctl:          return sys_ioctl(a1, a2, a3);
        case SYS_writev:         return sys_writev(a1, a2, a3);
        case SYS_access:         return -2; // ENOENT
        case SYS_pipe:           return -38;
        case SYS_dup:            return -38;
        case SYS_dup2:           return -38;
        case SYS_getpid:         return sys_getpid();
        case SYS_fork:           return -38;
        case SYS_execve:         return -38;
        case SYS_exit:           return sys_exit(a1);
        case SYS_exit_group:     return sys_exit(a1);
        case SYS_wait4:          return -10; // ECHILD
        case SYS_fcntl:          return sys_fcntl(a1, a2, a3);
        case SYS_getcwd:         if (a1 && a2 >= 2) { ((char*)a1)[0]='/'; ((char*)a1)[1]=0; return 1; } return -34;
        case SYS_chdir:          return 0;
        case SYS_getdents64:     return 0;
        case SYS_set_tid_address: return sys_set_tid_address(a1);
        case SYS_clock_gettime:  return sys_clock_gettime(a1, a2);
        case SYS_arch_prctl:     return sys_arch_prctl(a1, a2);
        case SYS_set_robust_list: return sys_set_robust_list(a1, a2);
        case SYS_getrandom:      return sys_getrandom(a1, a2, a3);
        case SYS_openat:         return -2; // ENOENT
        case SYS_open:           return -2; // ENOENT
        default:
            vga_puts("[SYSCALL] Unknown: ");
            vga_puthex(num);
            vga_puts("\n");
            return -38; // ENOSYS
    }
}
