#include <stddef.h>
#include <stdint.h>

#define VGA_WIDTH 80
#define VGA_HEIGHT 25
#define VGA_MEMORY ((volatile uint16_t *)0xB8000)
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
    write_at("ENPY OS", 10, 35, 0x0B);
    write_at("Experimental kernel booted successfully.", 12, 20, COLOR);
    write_at("32-bit x86 | GRUB Multiboot | Milestone 1", 14, 19, 0x07);
    write_at("This is a minimal development kernel, not a complete OS.", 16, 13, 0x0E);

    for (;;) {
        __asm__ volatile ("cli; hlt");
    }
}
