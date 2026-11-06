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

struct idt_ptr { uint16_t limit; uint32_t base; } __attribute__((packed));
static struct idt_entry idt[IDT_ENTRY_COUNT];
static struct idt_ptr idtp;

extern void idt_flush(uint32_t address);
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
static int wait_input_clear(void) {
    for (uint32_t i = 0; i < 100000; ++i)
        if ((inb(0x64) & 2) == 0) return 1;
    return 0;
}
static int wait_output_full(void) {
    for (uint32_t i = 0; i < 100000; ++i)
        if (inb(0x64) & 1) return 1;
    return 0;
}

static void set_gate(uint8_t n, uint32_t address) {
    idt[n].base_low = (uint16_t)address;
    idt[n].selector = KERNEL_CS;
    idt[n].zero = 0;
    idt[n].flags = IDT_GATE_PRESENT | IDT_GATE_TYPE_INT;
    idt[n].base_high = (uint16_t)(address >> 16);
}

void init_idt(void) {
    idtp.limit = sizeof(idt) - 1;
    idtp.base = (uint32_t)idt;
    for (unsigned i = 0; i < IDT_ENTRY_COUNT; ++i)
        set_gate((uint8_t)i, (uint32_t)isr_stub);
    set_gate(0x21, (uint32_t)irq1_stub);
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

/* Input devices are polled; mask all PIC IRQs to keep this prototype simple. */
void init_keyboard(void) {
    __asm__ volatile ("cli" ::: "memory");
    remap_pic();
    outb(0x21, 0xFF);
    outb(0xA1, 0xFF);
}

int keyboard_poll(void) {
    uint8_t status = inb(0x64);
    if ((status & 1) == 0 || (status & 0x20)) return 0;
    extern void keyboard_scancode_handler(uint8_t code);
    keyboard_scancode_handler(inb(0x60));
    return 1;
}

static int mouse_command(uint8_t command) {
    if (!wait_input_clear()) return 0;
    outb(0x64, 0xD4);
    if (!wait_input_clear()) return 0;
    outb(0x60, command);
    if (!wait_output_full()) return 0;
    uint8_t status = inb(0x64);
    uint8_t response = inb(0x60);
    return (status & 0x20) && response == 0xFA;
}

int init_mouse(void) {
    /* Enable the auxiliary PS/2 port and keep it available for polling. */
    if (!wait_input_clear()) return 0;
    outb(0x64, 0xA8);
    if (!wait_input_clear()) return 0;
    outb(0x64, 0x20);
    if (!wait_output_full()) return 0;
    uint8_t config = inb(0x60);
    /* Clear only the second-port clock-disable bit; keep IRQ12 disabled. */
    config = (uint8_t)(config & (uint8_t)~0x20);
    if (!wait_input_clear()) return 0;
    outb(0x64, 0x60);
    if (!wait_input_clear()) return 0;
    outb(0x60, config);
    if (!mouse_command(0xF6)) return 0; /* Set defaults. */
    return mouse_command(0xF4);         /* Enable streaming. */
}

int mouse_poll(int *dx, int *dy, uint8_t *buttons) {
    static uint8_t packet[3];
    static unsigned packet_index;
    uint8_t status = inb(0x64);
    if ((status & 1) == 0 || (status & 0x20) == 0) return 0;
    uint8_t data = inb(0x60);
    if (packet_index == 0) {
        if ((data & 0x08) == 0) return 0;
        packet[packet_index++] = data;
        return 0;
    }
    packet[packet_index++] = data;
    if (packet_index < 3) return 0;
    packet_index = 0;
    if (packet[0] & 0xC0) return 0;
    *dx = (int)(int8_t)packet[1];
    *dy = -(int)(int8_t)packet[2];
    *buttons = packet[0] & 0x07;
    return 1;
}

void keyboard_irq_handler(void) {
    extern void keyboard_scancode_handler(uint8_t code);
    keyboard_scancode_handler(inb(0x60));
    outb(0x20, 0x20);
}
void enable_interrupts(void) { __asm__ volatile ("sti" ::: "memory"); }
void disable_interrupts(void) { __asm__ volatile ("cli" ::: "memory"); }
