#include "../include/vfs.h"
#include "../include/vga.h"
#include "../include/keyboard.h"

static fd_entry_t fd_table[FD_MAX];

void vfs_init(void) {
    for (int i = 0; i < FD_MAX; i++) fd_table[i].type = VFS_TYPE_NONE;
    fd_table[FD_STDIN].type  = VFS_TYPE_CONSOLE;
    fd_table[FD_STDOUT].type = VFS_TYPE_CONSOLE;
    fd_table[FD_STDERR].type = VFS_TYPE_CONSOLE;
}

int64_t vfs_write(int fd, const void *buf, size_t count) {
    if (fd < 0 || fd >= FD_MAX) return -9; // EBADF
    if (fd_table[fd].type == VFS_TYPE_NONE) return -9;

    if (fd_table[fd].type == VFS_TYPE_CONSOLE) {
        const char *s = (const char *)buf;
        for (size_t i = 0; i < count; i++) {
            vga_putchar(s[i]);
        }
        return (int64_t)count;
    }
    return -9;
}

int64_t vfs_read(int fd, void *buf, size_t count) {
    if (fd < 0 || fd >= FD_MAX) return -9;
    if (fd_table[fd].type == VFS_TYPE_NONE) return -9;

    if (fd_table[fd].type == VFS_TYPE_CONSOLE && fd == FD_STDIN) {
        char *s = (char *)buf;
        for (size_t i = 0; i < count; i++) {
            s[i] = keyboard_getchar();
            // echo
            vga_putchar(s[i]);
            if (s[i] == '\n') return (int64_t)(i + 1);
        }
        return (int64_t)count;
    }
    return -9;
}

int vfs_close(int fd) {
    if (fd < 0 || fd >= FD_MAX) return -9;
    fd_table[fd].type = VFS_TYPE_NONE;
    return 0;
}

int64_t vfs_writev(int fd, const iovec_t *iov, int iovcnt) {
    int64_t total = 0;
    for (int i = 0; i < iovcnt; i++) {
        if (iov[i].iov_len == 0) continue;
        int64_t r = vfs_write(fd, iov[i].iov_base, iov[i].iov_len);
        if (r < 0) return r;
        total += r;
    }
    return total;
}
