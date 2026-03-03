#include "../include/keyboard.h"
#include "../include/idt.h"
#include "../include/vga.h"

#define KEYBOARD_DATA    0x60
#define KEYBOARD_STATUS  0x64
#define BUF_SIZE         256

static char buf[BUF_SIZE];
static volatile uint8_t buf_head = 0;
static volatile uint8_t buf_tail = 0;

static const char scancode_map[128] = {
    0,   27,  '1', '2', '3', '4', '5', '6', '7', '8', '9', '0', '-', '=', '\b',
    '\t','q',  'w', 'e', 'r', 't', 'y', 'u', 'i', 'o', 'p', '[', ']', '\n',
    0,   'a',  's', 'd', 'f', 'g', 'h', 'j', 'k', 'l', ';', '\'', '`',
    0,   '\\', 'z', 'x', 'c', 'v', 'b', 'n', 'm', ',', '.', '/', 0,
    '*', 0,   ' '
};

static const char scancode_map_shift[128] = {
    0,   27,  '!', '@', '#', '$', '%', '^', '&', '*', '(', ')', '_', '+', '\b',
    '\t','Q',  'W', 'E', 'R', 'T', 'Y', 'U', 'I', 'O', 'P', '{', '}', '\n',
    0,   'A',  'S', 'D', 'F', 'G', 'H', 'J', 'K', 'L', ':', '"', '~',
    0,   '|',  'Z', 'X', 'C', 'V', 'B', 'N', 'M', '<', '>', '?', 0,
    '*', 0,   ' '
};

static uint8_t shift_held = 0;
static uint8_t caps_lock  = 0;

static inline uint8_t inb(uint16_t port) {
    uint8_t val;
    __asm__ volatile("inb %1, %0" : "=a"(val) : "Nd"(port));
    return val;
}

static void keyboard_handler(regs_t *r) {
    (void)r;
    uint8_t sc = inb(KEYBOARD_DATA);

    // key release
    if (sc & 0x80) {
        sc &= 0x7F;
        if (sc == 0x2A || sc == 0x36) shift_held = 0;
        return;
    }

    // shift
    if (sc == 0x2A || sc == 0x36) { shift_held = 1; return; }
    // caps lock toggle
    if (sc == 0x3A) { caps_lock ^= 1; return; }

    if (sc >= 128) return;

    int use_upper = shift_held ^ caps_lock;
    char c = use_upper ? scancode_map_shift[sc] : scancode_map[sc];
    if (!c) return;

    uint8_t next = (buf_tail + 1) % BUF_SIZE;
    if (next != buf_head) {
        buf[buf_tail] = c;
        buf_tail = next;
    }
}

void keyboard_init(void) {
while (inb(KEYBOARD_STATUS) & 0x01) inb(KEYBOARD_DATA);
// enable keyboard
outb(KEYBOARD_STATUS, 0xAE);
    irq_register(1, keyboard_handler);
    vga_puts("[KBD] Initialized\n");
}

int keyboard_haschar(void) {
    return buf_head != buf_tail;
}

char keyboard_getchar(void) {
    while (!keyboard_haschar());
    char c = buf[buf_head];
    buf_head = (buf_head + 1) % BUF_SIZE;
    return c;
}
