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

static inline void io_wait(void) {
    outb(0x80, 0);
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

    for (uint16_t i = 0; i < IDT_ENTRY_COUNT; ++i) {
        idt_set_gate((uint8_t)i, (uint32_t)isr_stub, KERNEL_CS,
                     IDT_GATE_PRESENT | IDT_GATE_TYPE_INT);
    }
    /* IRQ1 after PIC remapping to vectors 0x20-0x2F. */
    idt_set_gate(0x21, (uint32_t)irq1_stub, KERNEL_CS,
                 IDT_GATE_PRESENT | IDT_GATE_TYPE_INT);
    idt_flush((uint32_t)&idtp);
}

static void remap_pic(void) {
    outb(0x20, 0x11); io_wait();
    outb(0xA0, 0x11); io_wait();
    outb(0x21, 0x20); io_wait();
    outb(0xA1, 0x28); io_wait();
    outb(0x21, 0x04); io_wait();
    outb(0xA1, 0x02); io_wait();
    outb(0x21, 0x01); io_wait();
    outb(0xA1, 0x01); io_wait();
}

void init_pic(void) {
    remap_pic();
    outb(0x21, 0xFF);
    outb(0xA1, 0xFF);
}

/* Wait for the PS/2 controller input buffer to become available. */
static int ps2_wait_input_clear(void) {
    for (uint32_t i = 0; i < 100000; ++i) {
        if ((inb(0x64) & 0x02) == 0) return 1;
    }
    return 0;
}

/* Wait for a byte from the PS/2 controller. */
static int ps2_wait_output_full(void) {
    for (uint32_t i = 0; i < 100000; ++i) {
        if (inb(0x64) & 0x01) return 1;
    }
    return 0;
}

void init_keyboard(void) {
    /* Keep interrupts disabled while PIC and PS/2 state is configured. */
    __asm__ volatile ("cli" ::: "memory");
    remap_pic();
    outb(0x21, 0xFF);
    outb(0xA1, 0xFF);

    /* Enable the first PS/2 port, then turn on its IRQ in the controller. */
    if (ps2_wait_input_clear()) outb(0x64, 0xAE);
    if (ps2_wait_input_clear()) {
        outb(0x64, 0x20);
        if (ps2_wait_output_full()) {
            uint8_t config = inb(0x60);
            config |= 0x01;       /* IRQ1 enabled */
            config &= (uint8_t)~0x10; /* first PS/2 port clock enabled */
            if (ps2_wait_input_clear()) {
                outb(0x64, 0x60);
                if (ps2_wait_input_clear()) outb(0x60, config);
            }
        }
    }

    /* Discard any stale scan byte before unmasking IRQ1. */
    if (inb(0x64) & 0x01) (void)inb(0x60);

    /* Unmask only IRQ1; keep timer and all other IRQs masked. */
    outb(0xA1, 0xFF);
    outb(0x21, 0xFD);
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
