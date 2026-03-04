#pragma once
#include <stdint.h>
#include <stddef.h>

#define FD_MAX    64
#define FD_STDIN  0
#define FD_STDOUT 1
#define FD_STDERR 2

#define VFS_TYPE_NONE    0
#define VFS_TYPE_CONSOLE 1

typedef struct {
    int      type;
    uint64_t offset;
    int      flags;
} fd_entry_t;

void vfs_init(void);

// Process fd table ops
int64_t vfs_write(int fd, const void *buf, size_t count);
int64_t vfs_read(int fd, void *buf, size_t count);
int     vfs_close(int fd);

// writev support
typedef struct {
    const void *iov_base;
    size_t      iov_len;
} iovec_t;

int64_t vfs_writev(int fd, const iovec_t *iov, int iovcnt);
