#include <stddef.h>
#include "interrupts.h"

#define VGA_MEMORY ((volatile uint16_t *)0xB8000)
#define VGA_WIDTH 80
#define VGA_HEIGHT 25
#define INPUT_CAPACITY 80

#define COLOR_DESKTOP 0x1F
#define COLOR_PANEL 0x70
#define COLOR_TEXT 0x0F
#define COLOR_ACCENT 0x0B

static char input[INPUT_CAPACITY];
static size_t input_length;
static int shift_pressed;
static int start_menu_open;

static void fill_row(size_t row, uint8_t color, char ch) {
    if (row >= VGA_HEIGHT) return;
    uint16_t cell = ((uint16_t)color << 8) | (uint8_t)ch;
    for (size_t column = 0; column < VGA_WIDTH; ++column)
        VGA_MEMORY[row * VGA_WIDTH + column] = cell;
}

static void write_at(const char *text, size_t row, size_t column, uint8_t color) {
    if (row >= VGA_HEIGHT || column >= VGA_WIDTH) return;
    while (*text && column < VGA_WIDTH) {
        VGA_MEMORY[row * VGA_WIDTH + column++] =
            ((uint16_t)color << 8) | (uint8_t)*text++;
    }
}

static void draw_desktop(void) {
    for (size_t row = 0; row < VGA_HEIGHT - 1; ++row)
        fill_row(row, COLOR_DESKTOP, ' ');

    fill_row(0, COLOR_PANEL, ' ');
    write_at("ENPY OS Desktop", 0, 2, COLOR_PANEL);
    write_at("F1: Start menu   F2: Help", 0, 52, COLOR_PANEL);

    write_at("[ Files ]", 4, 4, 0x1E);
    write_at("[ Terminal ]", 7, 4, 0x1E);
    write_at("[ Settings ]", 10, 4, 0x1E);
    write_at("Welcome to ENPY OS", 5, 28, 0x1F);
    write_at("A tiny hobby OS desktop prototype", 7, 28, 0x1F);
    write_at("Use the command line below. Type help for commands.", 9, 14, 0x1F);
    write_at("This prototype has no filesystem or Windows applications yet.", 11, 9, 0x1F);

    fill_row(20, 0x17, ' ');
    write_at("ENPY> ", 20, 1, 0x1F);
    write_at(input, 20, 7, 0x1F);

    fill_row(24, COLOR_PANEL, ' ');
    write_at("[Start]", 24, 1, 0x2F);
    write_at("ENPY OS", 24, 12, COLOR_PANEL);
    write_at("F1 toggles menu", 24, 62, COLOR_PANEL);

    if (start_menu_open) {
        for (size_t row = 13; row < 24; ++row)
            fill_row(row, 0x70, ' ');
        write_at("ENPY Start", 13, 3, 0x70);
        write_at("Available commands:", 15, 3, 0x70);
        write_at("help      - list commands", 16, 3, 0x70);
        write_at("about     - system information", 17, 3, 0x70);
        write_at("clear     - redraw desktop", 18, 3, 0x70);
        write_at("echo TEXT - print text", 19, 3, 0x70);
        write_at("Press F1 to close this menu.", 21, 3, 0x70);
    }
}

static int string_equals(const char *a, const char *b) {
    while (*a && *b && *a == *b) { ++a; ++b; }
    return *a == '\0' && *b == '\0';
}

static void run_command(void) {
    input[input_length] = '\0';
    start_menu_open = 0;
    draw_desktop();
    fill_row(22, COLOR_DESKTOP, ' ');

    if (string_equals(input, "help")) {
        write_at("Commands: help, about, clear, echo TEXT", 22, 1, COLOR_TEXT);
    } else if (string_equals(input, "about")) {
        write_at("ENPY OS is a small freestanding x86 hobby kernel.", 22, 1, COLOR_TEXT);
    } else if (string_equals(input, "clear")) {
        /* draw_desktop already cleared the screen */
    } else if (input_length >= 5 && input[0] == 'e' && input[1] == 'c' &&
               input[2] == 'h' && input[3] == 'o' && input[4] == ' ') {
        write_at(input + 5, 22, 1, COLOR_TEXT);
    } else if (input_length != 0) {
        write_at("Not available yet. Type help.", 22, 1, COLOR_TEXT);
    }

    input_length = 0;
    input[0] = '\0';
    draw_desktop();
}

static char scancode_to_char(uint8_t code, int shifted) {
    static const char normal[128] = {
        [0x02]='1',[0x03]='2',[0x04]='3',[0x05]='4',[0x06]='5',
        [0x07]='6',[0x08]='7',[0x09]='8',[0x0A]='9',[0x0B]='0',
        [0x0C]='-',[0x0D]='=',[0x0E]='\b',[0x10]='q',[0x11]='w',
        [0x12]='e',[0x13]='r',[0x14]='t',[0x15]='y',[0x16]='u',
        [0x17]='i',[0x18]='o',[0x19]='p',[0x1A]='[',[0x1B]=']',
        [0x1C]='\n',[0x1E]='a',[0x1F]='s',[0x20]='d',[0x21]='f',
        [0x22]='g',[0x23]='h',[0x24]='j',[0x25]='k',[0x26]='l',
        [0x27]=';',[0x28]='\'',[0x29]='`',[0x2B]='\\',[0x2C]='z',
        [0x2D]='x',[0x2E]='c',[0x2F]='v',[0x30]='b',[0x31]='n',
        [0x32]='m',[0x33]=',[',[0x34]='.',[0x35]='/',[0x39]=' '
    };
    static const char upper[128] = {
        [0x02]='!',[0x03]='@',[0x04]='#',[0x05]='$',[0x06]='%',
        [0x07]='^',[0x08]='&',[0x09]='*',[0x0A]='(',[0x0B]=')',
        [0x0C]='_',[0x0D]='+',[0x0E]='\b',[0x10]='Q',[0x11]='W',
        [0x12]='E',[0x13]='R',[0x14]='T',[0x15]='Y',[0x16]='U',
        [0x17]='I',[0x18]='O',[0x19]='P',[0x1A]='{',[0x1B]='}',
        [0x1C]='\n',[0x1E]='A',[0x1F]='S',[0x20]='D',[0x21]='F',
        [0x22]='G',[0x23]='H',[0x24]='J',[0x25]='K',[0x26]='L',
        [0x27]=':',[0x28]='"',[0x29]='~',[0x2B]='|',[0x2C]='Z',
        [0x2D]='X',[0x2E]='C',[0x2F]='V',[0x30]='B',[0x31]='N',
        [0x32]='M',[0x33]='<',[0x34]='>',[0x35]='?',[0x39]=' '
    };
    return shifted ? upper[code] : normal[code];
}

void keyboard_scancode_handler(uint8_t code) {
    if (code == 0x3B) { start_menu_open = !start_menu_open; draw_desktop(); return; }
    if (code == 0x3C) { start_menu_open = 1; draw_desktop(); return; }
    if (code == 0x2A || code == 0x36) { shift_pressed = 1; return; }
    if (code == 0xAA || code == 0xB6) { shift_pressed = 0; return; }
    if (code & 0x80) return;

    char c = scancode_to_char(code, shift_pressed);
    if (c == '\n') {
        run_command();
    } else if (c == '\b') {
        if (input_length) input[--input_length] = '\0';
        draw_desktop();
    } else if (c >= ' ' && c <= '~' && input_length < INPUT_CAPACITY - 1) {
        input[input_length++] = c;
        input[input_length] = '\0';
        draw_desktop();
    }
}

void kernel_main(void) {
    disable_interrupts();
    input[0] = '\0';
    init_idt();
    init_keyboard();
    draw_desktop();
    enable_interrupts();
    for (;;) __asm__ volatile ("hlt");
}
