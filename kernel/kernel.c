#include <stddef.h>
#include "interrupts.h"

#define VGA_MEMORY ((volatile uint16_t *)0xB8000)
#define VGA_WIDTH 80
#define VGA_HEIGHT 25
#define COLOR 0x0F
#define INPUT_CAPACITY 80

static size_t cursor_row;
static size_t cursor_column;
static char input[INPUT_CAPACITY];
static size_t input_length;
static int shift_pressed;

static void clear_screen(void) {
    const uint16_t blank = ((uint16_t)COLOR << 8) | ' ';
    for (size_t i = 0; i < VGA_WIDTH * VGA_HEIGHT; ++i) {
        VGA_MEMORY[i] = blank;
    }
    cursor_row = 0;
    cursor_column = 0;
}

static void scroll_if_needed(void) {
    if (cursor_row < VGA_HEIGHT) {
        return;
    }
    for (size_t row = 1; row < VGA_HEIGHT; ++row) {
        for (size_t column = 0; column < VGA_WIDTH; ++column) {
            VGA_MEMORY[(row - 1) * VGA_WIDTH + column] =
                VGA_MEMORY[row * VGA_WIDTH + column];
        }
    }
    const uint16_t blank = ((uint16_t)COLOR << 8) | ' ';
    for (size_t column = 0; column < VGA_WIDTH; ++column) {
        VGA_MEMORY[(VGA_HEIGHT - 1) * VGA_WIDTH + column] = blank;
    }
    cursor_row = VGA_HEIGHT - 1;
}

static void put_char(char c) {
    if (c == '\n') {
        cursor_column = 0;
        ++cursor_row;
        scroll_if_needed();
        return;
    }
    if (c == '\b') {
        if (cursor_column > 0) {
            --cursor_column;
            VGA_MEMORY[cursor_row * VGA_WIDTH + cursor_column] =
                ((uint16_t)COLOR << 8) | ' ';
        }
        return;
    }
    VGA_MEMORY[cursor_row * VGA_WIDTH + cursor_column] =
        ((uint16_t)COLOR << 8) | (uint8_t)c;
    if (++cursor_column == VGA_WIDTH) {
        cursor_column = 0;
        ++cursor_row;
        scroll_if_needed();
    }
}

static void print(const char *text) {
    while (*text) {
        put_char(*text++);
    }
}

static int string_equals(const char *a, const char *b) {
    while (*a && *b && *a == *b) {
        ++a;
        ++b;
    }
    return *a == '\0' && *b == '\0';
}

static void prompt(void) {
    print("enpy> ");
}

static void run_command(void) {
    input[input_length] = '\0';
    put_char('\n');

    if (string_equals(input, "help")) {
        print("Commands: help, about, clear, echo <text>\n");
    } else if (string_equals(input, "about")) {
        print("ENPY OS: a small freestanding x86 kernel.\n");
    } else if (string_equals(input, "clear")) {
        clear_screen();
    } else if (input_length >= 5 && input[0] == 'e' && input[1] == 'c' &&
               input[2] == 'h' && input[3] == 'o' && input[4] == ' ') {
        print(input + 5);
        put_char('\n');
    } else if (input_length != 0) {
        print("Unknown command. Type help.\n");
    }

    input_length = 0;
    prompt();
}

static char scancode_to_char(uint8_t code, int shifted) {
    static const char normal[128] = {
        [0x02]='1',[0x03]='2',[0x04]='3',[0x05]='4',[0x06]='5',
        [0x07]='6',[0x08]='7',[0x09]='8',[0x0A]='9',[0x0B]='0',
        [0x0C]='-',[0x0D]='=',[0x0E]='\b',[0x0F]='\t',
        [0x10]='q',[0x11]='w',[0x12]='e',[0x13]='r',[0x14]='t',
        [0x15]='y',[0x16]='u',[0x17]='i',[0x18]='o',[0x19]='p',
        [0x1A]='[',[0x1B]=']',[0x1C]='\n',[0x1E]='a',[0x1F]='s',
        [0x20]='d',[0x21]='f',[0x22]='g',[0x23]='h',[0x24]='j',
        [0x25]='k',[0x26]='l',[0x27]=';',[0x28]='\'',[0x29]='`',
        [0x2B]='\\',[0x2C]='z',[0x2D]='x',[0x2E]='c',[0x2F]='v',
        [0x30]='b',[0x31]='n',[0x32]='m',[0x33]=',[',[0x34]='.',
        [0x35]='/',[0x39]=' '
    };
    static const char shifted_chars[128] = {
        [0x02]='!',[0x03]='@',[0x04]='#',[0x05]='$',[0x06]='%',
        [0x07]='^',[0x08]='&',[0x09]='*',[0x0A]='(',[0x0B]=')',
        [0x0C]='_',[0x0D]='+',[0x0E]='\b',[0x0F]='\t',
        [0x10]='Q',[0x11]='W',[0x12]='E',[0x13]='R',[0x14]='T',
        [0x15]='Y',[0x16]='U',[0x17]='I',[0x18]='O',[0x19]='P',
        [0x1A]='{',[0x1B]='}',[0x1C]='\n',[0x1E]='A',[0x1F]='S',
        [0x20]='D',[0x21]='F',[0x22]='G',[0x23]='H',[0x24]='J',
        [0x25]='K',[0x26]='L',[0x27]=':',[0x28]='"',[0x29]='~',
        [0x2B]='|',[0x2C]='Z',[0x2D]='X',[0x2E]='C',[0x2F]='V',
        [0x30]='B',[0x31]='N',[0x32]='M',[0x33]='<',[0x34]='>',
        [0x35]='?',[0x39]=' '
    };
    return shifted ? shifted_chars[code] : normal[code];
}

void keyboard_scancode_handler(uint8_t code) {
    if (code == 0x2A || code == 0x36) {
        shift_pressed = 1;
        return;
    }
    if (code == 0xAA || code == 0xB6) {
        shift_pressed = 0;
        return;
    }
    if (code & 0x80) {
        return;
    }

    char c = scancode_to_char(code, shift_pressed);
    if (c == '\n') {
        run_command();
    } else if (c == '\b') {
        if (input_length > 0) {
            --input_length;
            put_char('\b');
        }
    } else if (c >= ' ' && c <= '~' && input_length < INPUT_CAPACITY - 1) {
        input[input_length++] = c;
        put_char(c);
    }
}

void kernel_main(void) {
    disable_interrupts();
    clear_screen();
    print("ENPY OS - interactive kernel console\n");
    print("Type help and press Enter.\n\n");
    init_idt();
    init_keyboard();
    prompt();
    enable_interrupts();

    for (;;) {
        __asm__ volatile ("hlt");
    }
}
