#pragma once
#include <stdint.h>

void syscall_init(void);

// Syscall numbers (Linux-compatible for musl)
#define SYS_read        0
#define SYS_write       1
#define SYS_open        2
#define SYS_close       3
#define SYS_stat        4
#define SYS_fstat       5
#define SYS_lseek       8
#define SYS_mmap        9
#define SYS_mprotect    10
#define SYS_munmap      11
#define SYS_brk         12
#define SYS_ioctl       16
#define SYS_writev      20
#define SYS_access      21
#define SYS_pipe        22
#define SYS_dup         32
#define SYS_dup2        33
#define SYS_getpid      39
#define SYS_fork        57
#define SYS_execve      59
#define SYS_exit        60
#define SYS_wait4       61
#define SYS_fcntl       72
#define SYS_getcwd      79
#define SYS_chdir       80
#define SYS_getdents64  217
#define SYS_exit_group  231
#define SYS_openat      257
#define SYS_set_tid_address 218
#define SYS_clock_gettime   228
#define SYS_arch_prctl  158
#define SYS_set_robust_list 273
#define SYS_getrandom   318
