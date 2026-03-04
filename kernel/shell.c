#include "../include/shell.h"
#include "../include/keyboard.h"
#include "../include/vga.h"
#include "../include/pmm.h"
#include "../include/heap.h"
#include "../include/elf.h"
#include "../include/process.h"
#include "../include/string.h"

#define CMD_LEN 256

static char cmd[CMD_LEN];
static int  cmd_pos = 0;

// Embedded initramfs (set by multiboot module or built-in test)
extern const uint8_t _binary_initramfs_start[] __attribute__((weak));
extern const uint8_t _binary_initramfs_end[] __attribute__((weak));

static void shell_puts(const char *s) { vga_puts(s); }

static void cmd_clear_buf(void) {
    cmd_pos = 0;
    cmd[0]  = 0;
}

static int streq(const char *a, const char *b) {
    return strcmp(a, b) == 0;
}

static int starts_with(const char *s, const char *prefix) {
    return strncmp(s, prefix, strlen(prefix)) == 0;
}

static void vga_putuint(uint64_t n) {
    if (n == 0) { vga_putchar('0'); return; }
    char buf[21]; int i = 0;
    while (n) { buf[i++] = '0' + (n % 10); n /= 10; }
    for (int j = i-1; j >= 0; j--) vga_putchar(buf[j]);
}

static void cmd_help(void) {
    shell_puts("Commands: help, clear, version, meminfo, exec\n");
}

static void cmd_meminfo(void) {
    shell_puts("Free pages: ");
    vga_putuint(pmm_free_pages());
    shell_puts(" (");
    vga_putuint(pmm_free_pages() * 4 / 1024);
    shell_puts(" MB)\n");
}

static void cmd_exec(void) {
    // Check for embedded initramfs
    if (&_binary_initramfs_start[0] == (void*)0 || &_binary_initramfs_end[0] == (void*)0 ||
        &_binary_initramfs_start[0] == &_binary_initramfs_end[0]) {
        shell_puts("No initramfs loaded. Use GRUB module to load an ELF.\n");
        shell_puts("Or build with an embedded test binary.\n");
        return;
    }

    uint64_t size = (uint64_t)(_binary_initramfs_end - _binary_initramfs_start);
    shell_puts("Loading ELF (");
    vga_putuint(size);
    shell_puts(" bytes)...\n");

    process_t *p = proc_create();
    if (!p) {
        shell_puts("Failed to create process\n");
        return;
    }

    uint64_t entry = elf_load(p, _binary_initramfs_start, size);
    if (!entry) {
        shell_puts("ELF load failed\n");
        return;
    }

    p->ctx.rip = entry;
    shell_puts("Entering usermode...\n");
    proc_enter_usermode(p);
}

static void handle_cmd(void) {
    if (cmd_pos == 0) return;
    cmd[cmd_pos] = 0;

    if (streq(cmd, "help")) {
        cmd_help();
    } else if (streq(cmd, "clear")) {
        vga_clear();
    } else if (streq(cmd, "version")) {
        shell_puts("KOKIRI x86_64 dev build (musl-ready)\n");
    } else if (streq(cmd, "meminfo")) {
        cmd_meminfo();
    } else if (streq(cmd, "exec") || starts_with(cmd, "exec ")) {
        cmd_exec();
    } else {
        shell_puts("Unknown command: ");
        shell_puts(cmd);
        shell_puts("\n");
    }
}

void shell_run(void) {
    vga_set_color(VGA_LIGHT_GREEN, VGA_BLACK);
    shell_puts("KOKIRI debug shell - type 'help'\n");
    vga_set_color(VGA_WHITE, VGA_BLACK);

    while (1) {
        vga_set_color(VGA_LIGHT_CYAN, VGA_BLACK);
        shell_puts("kokiri> ");
        vga_set_color(VGA_WHITE, VGA_BLACK);
        cmd_clear_buf();

        while (1) {
            char c = keyboard_getchar();
            if (c == '\n') {
                vga_putchar('\n');
                handle_cmd();
                break;
            } else if (c == '\b') {
                if (cmd_pos > 0) {
                    cmd_pos--;
                    vga_putchar('\b');
                }
            } else if (cmd_pos < CMD_LEN - 1) {
                cmd[cmd_pos++] = c;
                vga_putchar(c);
            }
        }
    }
}
