#include <stddef.h>
#include "interrupts.h"

#define VGA_MEMORY ((volatile uint16_t *)0xB8000)
#define VGA_WIDTH 80
#define VGA_HEIGHT 25
#define ATTR(bg, fg) ((uint8_t)(((bg) << 4) | (fg)))
#define BLUE ATTR(1, 15)
#define PANEL ATTR(7, 0)
#define HIGHLIGHT ATTR(1, 14)
#define GREEN ATTR(1, 10)

#define SCREEN_MENU 0
#define SCREEN_INSTALLER 1
#define SCREEN_DESKTOP 2

static int screen = SCREEN_MENU;

static void clear_screen(uint8_t attr) {
    uint16_t blank = ((uint16_t)attr << 8) | ' ';
    for (size_t i = 0; i < VGA_WIDTH * VGA_HEIGHT; ++i)
        VGA_MEMORY[i] = blank;
}

static void write_at(size_t x, size_t y, const char *text, uint8_t attr) {
    if (y >= VGA_HEIGHT || x >= VGA_WIDTH) return;
    while (*text && x < VGA_WIDTH)
        VGA_MEMORY[y * VGA_WIDTH + x++] = ((uint16_t)attr << 8) | (uint8_t)*text++;
}

static void draw_menu(void) {
    clear_screen(BLUE);
    for (size_t x = 0; x < VGA_WIDTH; ++x)
        VGA_MEMORY[x] = ((uint16_t)PANEL << 8) | ' ';
    write_at(2, 1, "ENPY OS | STARTUP", PANEL);
    write_at(28, 6, "Welcome to ENPY OS", HIGHLIGHT);
    write_at(21, 9, "Press 1 for installer preview", BLUE);
    write_at(21, 11, "Press 2 to start the desktop", BLUE);
    write_at(21, 15, "Keyboard controls: press the number key", GREEN);
    write_at(21, 17, "Mouse input is not supported yet", BLUE);
}

static void draw_installer(void) {
    clear_screen(BLUE);
    for (size_t x = 0; x < VGA_WIDTH; ++x)
        VGA_MEMORY[x] = ((uint16_t)PANEL << 8) | ' ';
    write_at(2, 1, "ENPY OS | INSTALLER PREVIEW", PANEL);
    write_at(18, 7, "Installer preview", HIGHLIGHT);
    write_at(10, 10, "This build cannot partition or install to a disk yet.", BLUE);
    write_at(10, 12, "No disk will be changed. Press Enter for the desktop.", BLUE);
    write_at(10, 15, "Press Esc to return to the startup menu.", GREEN);
}

static void draw_desktop(void) {
    clear_screen(ATTR(1, 15));
    write_at(0, 0, " ENPY OS Desktop                                                     ", PANEL);
    write_at(3, 4, "[ Files ]", HIGHLIGHT);
    write_at(3, 7, "[ Terminal ]", HIGHLIGHT);
    write_at(3, 10, "[ About ]", HIGHLIGHT);
    write_at(22, 6, "Welcome to the ENPY OS desktop.", BLUE);
    write_at(22, 8, "This is a keyboard-controlled text-mode prototype.", BLUE);
    write_at(22, 10, "Graphics, mouse, and real applications are not implemented.", BLUE);
    write_at(0, 24, " ENPY | Press Esc to return to startup menu ", PANEL);
}

void keyboard_scancode_handler(uint8_t code) {
    if (code & 0x80) return; /* Ignore key releases. */

    if (screen == SCREEN_MENU) {
        if (code == 0x02) {
            screen = SCREEN_INSTALLER;
            draw_installer();
        } else if (code == 0x03) {
            screen = SCREEN_DESKTOP;
            draw_desktop();
        }
        return;
    }

    if (screen == SCREEN_INSTALLER) {
        if (code == 0x1C) {
            screen = SCREEN_DESKTOP;
            draw_desktop();
        } else if (code == 0x01) {
            screen = SCREEN_MENU;
            draw_menu();
        }
        return;
    }

    if (screen == SCREEN_DESKTOP && code == 0x01) {
        screen = SCREEN_MENU;
        draw_menu();
    }
}

void kernel_main(void) {
    disable_interrupts();
    init_idt();
    init_keyboard();
    screen = SCREEN_MENU;
    draw_menu();
    enable_interrupts();
    for (;;) __asm__ volatile ("hlt");
}
