#include "../include/shell.h"
#include "../include/keyboard.h"
#include "../include/vga.h"

#define CMD_LEN 256

static char cmd[CMD_LEN];
static int  cmd_pos = 0;

static void shell_puts(const char *s) { vga_puts(s); }

static void cmd_clear(void) {
    cmd_pos = 0;
    cmd[0]  = 0;
}

static int streq(const char *a, const char *b) {
    while (*a && *b) if (*a++ != *b++) return 0;
    return *a == *b;
}

static void handle_cmd(void) {
    if (cmd_pos == 0) return;
    cmd[cmd_pos] = 0;

    if (streq(cmd, "help")) {
        shell_puts("Commands: help, clear, version\n");
    } else if (streq(cmd, "clear")) {
        vga_clear();
    } else if (streq(cmd, "version")) {
        shell_puts("KOKIRI x86_64 dev build\n");
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
        cmd_clear();

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
