#include <stddef.h>
#include "interrupts.h"

#define VGA_MEMORY ((volatile uint16_t *)0xB8000)
#define VGA_WIDTH 80
#define VGA_HEIGHT 25
#define COLOR 0x0F

static void clear_screen(void) {
    const uint16_t blank = ((uint16_t)COLOR << 8) | ' ';
    for (size_t i = 0; i < VGA_WIDTH * VGA_HEIGHT; ++i) {
        VGA_MEMORY[i] = blank;
    }
}

static void write_at(const char *text, size_t row, size_t column, uint8_t color) {
    size_t i = 0;
    while (text[i] != '\0' && column + i < VGA_WIDTH) {
        VGA_MEMORY[row * VGA_WIDTH + column + i] = ((uint16_t)color << 8) | (uint8_t)text[i];
        ++i;
    }
}

void kernel_main(void) {
    clear_screen();
    write_at("ENPY OS", 8, 34, 0x0B);
    write_at("Milestone 2: interrupt infrastructure", 11, 18, 0x07);
    write_at("Loading IDT and PIC setup...", 13, 20, 0x0E);

    init_idt();
    init_keyboard();
    enable_interrupts();

    write_at("Interrupts enabled.", 15, 29, 0x0A);
    write_at("Kernel is alive and waiting.", 17, 23, 0x0F);

    for (;;) {
        __asm__ volatile ("cli; hlt");
    }
}
