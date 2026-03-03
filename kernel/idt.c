#include "../include/idt.h"
#include "../include/vga.h"
#include <stddef.h>
#include <stdint.h>

#define IDT_ENTRIES 256
#define PIC1      0x20
#define PIC2      0xA0
#define PIC1_DATA 0x21
#define PIC2_DATA 0xA1

typedef struct __attribute__((packed)) {
    uint16_t base_low;
    uint16_t sel;
    uint8_t  ist;
    uint8_t  flags;
    uint16_t base_mid;
    uint32_t base_high;
    uint32_t zero;
} idt_entry_t;

typedef struct __attribute__((packed)) {
    uint16_t limit;
    uint64_t base;
} idt_ptr_t;

static idt_entry_t idt[IDT_ENTRIES];
static idt_ptr_t   idt_ptr;
static void       *irq_handlers[16];

extern void idt_flush(uint64_t);
extern void isr0(void);  extern void isr1(void);  extern void isr2(void);
extern void isr3(void);  extern void isr4(void);  extern void isr5(void);
extern void isr6(void);  extern void isr7(void);  extern void isr8(void);
extern void isr9(void);  extern void isr10(void); extern void isr11(void);
extern void isr12(void); extern void isr13(void); extern void isr14(void);
extern void isr15(void); extern void isr16(void); extern void isr17(void);
extern void isr18(void); extern void isr19(void); extern void isr20(void);
extern void isr21(void); extern void isr22(void); extern void isr23(void);
extern void isr24(void); extern void isr25(void); extern void isr26(void);
extern void isr27(void); extern void isr28(void); extern void isr29(void);
extern void isr30(void); extern void isr31(void);
extern void irq0(void);  extern void irq1(void);  extern void irq2(void);
extern void irq3(void);  extern void irq4(void);  extern void irq5(void);
extern void irq6(void);  extern void irq7(void);  extern void irq8(void);
extern void irq9(void);  extern void irq10(void); extern void irq11(void);
extern void irq12(void); extern void irq13(void); extern void irq14(void);
extern void irq15(void);

static inline void outb(uint16_t port, uint8_t val) {
    __asm__ volatile("outb %0, %1" : : "a"(val), "Nd"(port));
}

static void idt_set(int i, uint64_t base, uint16_t sel, uint8_t flags) {
    idt[i].base_low  = base & 0xFFFF;
    idt[i].base_mid  = (base >> 16) & 0xFFFF;
    idt[i].base_high = (base >> 32) & 0xFFFFFFFF;
    idt[i].sel = sel; idt[i].ist = 0;
    idt[i].flags = flags; idt[i].zero = 0;
}

static void pic_remap(void) {
    outb(PIC1, 0x11);      outb(PIC2, 0x11);
    outb(PIC1_DATA, 0x20); outb(PIC2_DATA, 0x28);
    outb(PIC1_DATA, 0x04); outb(PIC2_DATA, 0x02);
    outb(PIC1_DATA, 0x01); outb(PIC2_DATA, 0x01);
    outb(PIC1_DATA, 0x00); outb(PIC2_DATA, 0x00);
}

void idt_init(void) {
    idt_ptr.limit = sizeof(idt) - 1;
    idt_ptr.base  = (uint64_t)&idt;
    idt_set(0,(uint64_t)isr0,0x08,0x8E);   idt_set(1,(uint64_t)isr1,0x08,0x8E);
    idt_set(2,(uint64_t)isr2,0x08,0x8E);   idt_set(3,(uint64_t)isr3,0x08,0x8E);
    idt_set(4,(uint64_t)isr4,0x08,0x8E);   idt_set(5,(uint64_t)isr5,0x08,0x8E);
    idt_set(6,(uint64_t)isr6,0x08,0x8E);   idt_set(7,(uint64_t)isr7,0x08,0x8E);
    idt_set(8,(uint64_t)isr8,0x08,0x8E);   idt_set(9,(uint64_t)isr9,0x08,0x8E);
    idt_set(10,(uint64_t)isr10,0x08,0x8E); idt_set(11,(uint64_t)isr11,0x08,0x8E);
    idt_set(12,(uint64_t)isr12,0x08,0x8E); idt_set(13,(uint64_t)isr13,0x08,0x8E);
    idt_set(14,(uint64_t)isr14,0x08,0x8E); idt_set(15,(uint64_t)isr15,0x08,0x8E);
    idt_set(16,(uint64_t)isr16,0x08,0x8E); idt_set(17,(uint64_t)isr17,0x08,0x8E);
    idt_set(18,(uint64_t)isr18,0x08,0x8E); idt_set(19,(uint64_t)isr19,0x08,0x8E);
    idt_set(20,(uint64_t)isr20,0x08,0x8E); idt_set(21,(uint64_t)isr21,0x08,0x8E);
    idt_set(22,(uint64_t)isr22,0x08,0x8E); idt_set(23,(uint64_t)isr23,0x08,0x8E);
    idt_set(24,(uint64_t)isr24,0x08,0x8E); idt_set(25,(uint64_t)isr25,0x08,0x8E);
    idt_set(26,(uint64_t)isr26,0x08,0x8E); idt_set(27,(uint64_t)isr27,0x08,0x8E);
    idt_set(28,(uint64_t)isr28,0x08,0x8E); idt_set(29,(uint64_t)isr29,0x08,0x8E);
    idt_set(30,(uint64_t)isr30,0x08,0x8E); idt_set(31,(uint64_t)isr31,0x08,0x8E);
    pic_remap();
    idt_set(32,(uint64_t)irq0,0x08,0x8E);  idt_set(33,(uint64_t)irq1,0x08,0x8E);
    idt_set(34,(uint64_t)irq2,0x08,0x8E);  idt_set(35,(uint64_t)irq3,0x08,0x8E);
    idt_set(36,(uint64_t)irq4,0x08,0x8E);  idt_set(37,(uint64_t)irq5,0x08,0x8E);
    idt_set(38,(uint64_t)irq6,0x08,0x8E);  idt_set(39,(uint64_t)irq7,0x08,0x8E);
    idt_set(40,(uint64_t)irq8,0x08,0x8E);  idt_set(41,(uint64_t)irq9,0x08,0x8E);
    idt_set(42,(uint64_t)irq10,0x08,0x8E); idt_set(43,(uint64_t)irq11,0x08,0x8E);
    idt_set(44,(uint64_t)irq12,0x08,0x8E); idt_set(45,(uint64_t)irq13,0x08,0x8E);
    idt_set(46,(uint64_t)irq14,0x08,0x8E); idt_set(47,(uint64_t)irq15,0x08,0x8E);
    idt_flush((uint64_t)&idt_ptr);
    vga_puts("[IDT] Initialized\n");
}

static const char *exceptions[] = {
    "Division By Zero","Debug","NMI","Breakpoint","Overflow","Out of Bounds",
    "Invalid Opcode","No FPU","Double Fault","FPU Segment Overrun","Bad TSS",
    "Segment Not Present","Stack Fault","General Protection Fault","Page Fault",
    "Unknown","FPU Fault","Alignment Check","Machine Check","SIMD FPU Fault"
};

void isr_handler(regs_t *r) {
    vga_set_color(VGA_RED, VGA_BLACK);
    vga_puts("[EXCEPTION] ");
    if (r->int_no < 20) vga_puts(exceptions[r->int_no]);
    else vga_puts("Unknown");
    vga_puts("\n");
    vga_set_color(VGA_WHITE, VGA_BLACK);
    for (;;) __asm__("hlt");
}

void irq_register(uint8_t irq, void (*handler)(regs_t*)) {
    irq_handlers[irq] = handler;
}

void irq_handler(regs_t *r) {
    if (r->int_no >= 40) outb(PIC2, 0x20);
    outb(PIC1, 0x20);
    uint8_t irq = r->int_no - 32;
    void (*handler)(regs_t*) = irq_handlers[irq];
    if (handler) handler(r);
}
