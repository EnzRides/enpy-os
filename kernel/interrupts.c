#include <stdint.h>

#define IDT_ENTRY_COUNT 256
#define KERNEL_CS 0x08
#define IDT_GATE_PRESENT 0x80
#define IDT_GATE_TYPE_INT 0x0E

struct idt_entry {
    uint16_t base_low;
    uint16_t selector;
    uint8_t zero;
    uint8_t flags;
    uint16_t base_high;
} __attribute__((packed));

struct idt_ptr {
    uint16_t limit;
    uint32_t base;
} __attribute__((packed));

static struct idt_entry idt[IDT_ENTRY_COUNT];
static struct idt_ptr idtp;

extern void idt_flush(uint32_t idt_ptr_address);
extern void isr_stub(void);
extern void irq1_stub(void);

static inline void outb(uint16_t port, uint8_t value) {
    __asm__ volatile ("outb %0, %1" : : "a"(value), "Nd"(port));
}

static inline uint8_t inb(uint16_t port) {
    uint8_t value;
    __asm__ volatile ("inb %1, %0" : "=a"(value) : "Nd"(port));
    return value;
}

static void idt_set_gate(uint8_t num, uint32_t base, uint16_t selector, uint8_t flags) {
    idt[num].base_low = (uint16_t)(base & 0xFFFF);
    idt[num].selector = selector;
    idt[num].zero = 0;
    idt[num].flags = flags;
    idt[num].base_high = (uint16_t)((base >> 16) & 0xFFFF);
}

void init_idt(void) {
    idtp.limit = sizeof(idt) - 1;
    idtp.base = (uint32_t)idt;
    for (uint16_t i = 0; i < IDT_ENTRY_COUNT; ++i)
        idt_set_gate((uint8_t)i, (uint32_t)isr_stub, KERNEL_CS,
                     IDT_GATE_PRESENT | IDT_GATE_TYPE_INT);
    idt_set_gate(0x21, (uint32_t)irq1_stub, KERNEL_CS,
                 IDT_GATE_PRESENT | IDT_GATE_TYPE_INT);
    idt_flush((uint32_t)&idtp);
}

static void remap_pic(void) {
    outb(0x20, 0x11); outb(0x80, 0);
    outb(0xA0, 0x11); outb(0x80, 0);
    outb(0x21, 0x20); outb(0x80, 0);
    outb(0xA1, 0x28); outb(0x80, 0);
    outb(0x21, 0x04); outb(0x80, 0);
    outb(0xA1, 0x02); outb(0x80, 0);
    outb(0x21, 0x01); outb(0x80, 0);
    outb(0xA1, 0x01); outb(0x80, 0);
}

void init_pic(void) {
    remap_pic();
    outb(0x21, 0xFF);
    outb(0xA1, 0xFF);
}

/* Polling avoids enabling hardware IRQs until their handler path is verified. */
int keyboard_poll(void) {
    if ((inb(0x64) & 0x01) == 0) return 0;
    uint8_t scancode = inb(0x60);
    extern void keyboard_scancode_handler(uint8_t code);
    keyboard_scancode_handler(scancode);
    return 1;
}

void init_keyboard(void) {
    /* Keep legacy callers safe: keyboard input is polled, not IRQ driven. */
    __asm__ volatile ("cli" ::: "memory");
    remap_pic();
    outb(0x21, 0xFF);
    outb(0xA1, 0xFF);
}

void keyboard_irq_handler(void) {
    uint8_t scancode = inb(0x60);
    extern void keyboard_scancode_handler(uint8_t code);
    keyboard_scancode_handler(scancode);
    outb(0x20, 0x20);
}

void enable_interrupts(void) {
    __asm__ volatile ("sti" ::: "memory");
}

void disable_interrupts(void) {
    __asm__ volatile ("cli" ::: "memory");
}
