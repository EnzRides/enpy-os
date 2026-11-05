#include <stddef.h>
#include "interrupts.h"

#define VGA_MEMORY ((volatile uint16_t *)0xB8000)
#define VGA_WIDTH 80
#define VGA_HEIGHT 25
#define INPUT_CAPACITY 80
#define USER_CAPACITY 24

#define ATTR(bg, fg) ((uint8_t)(((bg) << 4) | (fg)))
#define WHITE_ON_BLUE ATTR(1, 15)
#define WHITE_ON_DARK ATTR(0, 15)
#define PANEL_ATTR ATTR(7, 0)
#define CYAN_ON_BLUE ATTR(1, 11)
#define YELLOW_ON_BLUE ATTR(1, 14)
#define GREEN_ON_BLUE ATTR(1, 10)
#define WHITE_ON_GREEN ATTR(2, 15)

static char input[INPUT_CAPACITY];
static size_t input_length;
static char username[USER_CAPACITY] = "Guest";
static int shift_pressed;
static int screen;
static int active_app;
static int command_message;

#define SCREEN_SETUP 0
#define SCREEN_INSTALL 1
#define SCREEN_LOGIN 2
#define SCREEN_DESKTOP 3

#define APP_NONE 0
#define APP_FILES 1
#define APP_TERMINAL 2
#define APP_ABOUT 3

static void cell(size_t x, size_t y, char ch, uint8_t attr) {
    if (x < VGA_WIDTH && y < VGA_HEIGHT)
        VGA_MEMORY[y * VGA_WIDTH + x] = ((uint16_t)attr << 8) | (uint8_t)ch;
}

static void fill_rect(size_t x, size_t y, size_t width, size_t height,
                      char ch, uint8_t attr) {
    for (size_t row = 0; row < height && y + row < VGA_HEIGHT; ++row)
        for (size_t col = 0; col < width && x + col < VGA_WIDTH; ++col)
            cell(x + col, y + row, ch, attr);
}

static void text_at(size_t x, size_t y, const char *text, uint8_t attr) {
    while (*text && x < VGA_WIDTH) cell(x++, y, *text++, attr);
}

static void centered(size_t y, const char *text, uint8_t attr) {
    size_t length = 0;
    while (text[length]) ++length;
    text_at(length < VGA_WIDTH ? (VGA_WIDTH - length) / 2 : 0, y, text, attr);
}

static int equals(const char *a, const char *b) {
    while (*a && *b && *a == *b) { ++a; ++b; }
    return *a == '\0' && *b == '\0';
}

static int starts_with(const char *text, const char *prefix) {
    while (*prefix && *text == *prefix) { ++text; ++prefix; }
    return *prefix == '\0';
}

static void draw_setup(void) {
    fill_rect(0, 0, VGA_WIDTH, VGA_HEIGHT, ' ', WHITE_ON_BLUE);
    fill_rect(0, 0, VGA_WIDTH, 3, ' ', PANEL_ATTR);
    text_at(2, 1, "ENPY OS  |  Setup", PANEL_ATTR);
    text_at(61, 1, "x86 edition", PANEL_ATTR);
    centered(6, "Welcome to ENPY OS", YELLOW_ON_BLUE);
    centered(8, "A small hobby operating system", WHITE_ON_BLUE);
    centered(11, "Choose how to continue:", CYAN_ON_BLUE);
    text_at(22, 14, "[1]  Install preview", WHITE_ON_BLUE);
    text_at(22, 16, "[2]  Try the live desktop", WHITE_ON_BLUE);
    centered(20, "Keyboard: press 1 or 2", GREEN_ON_BLUE);
    centered(23, "This is an experimental text-mode OS prototype.", WHITE_ON_BLUE);
}

static void draw_install(void) {
    fill_rect(0, 0, VGA_WIDTH, VGA_HEIGHT, ' ', WHITE_ON_BLUE);
    fill_rect(0, 0, VGA_WIDTH, 3, ' ', PANEL_ATTR);
    text_at(2, 1, "ENPY OS  |  Installer", PANEL_ATTR);
    centered(5, "Installation preview", YELLOW_ON_BLUE);
    text_at(13, 9, "Welcome, early tester.", WHITE_ON_BLUE);
    text_at(13, 11, "The installer interface is a preview only.", WHITE_ON_BLUE);
    text_at(13, 13, "Disk drivers, partitioning, and file installation", WHITE_ON_BLUE);
    text_at(13, 14, "are not implemented yet; no disk will be changed.", WHITE_ON_BLUE);
    text_at(13, 17, "Next: continue to the demo sign-in screen.", CYAN_ON_BLUE);
    centered(21, "Press Enter to continue  |  Esc to go back", GREEN_ON_BLUE);
}

static void draw_login(void) {
    fill_rect(0, 0, VGA_WIDTH, VGA_HEIGHT, ' ', WHITE_ON_BLUE);
    fill_rect(0, 0, VGA_WIDTH, 3, ' ', PANEL_ATTR);
    text_at(2, 1, "ENPY OS  |  Sign in", PANEL_ATTR);
    centered(6, "ENPY OS", YELLOW_ON_BLUE);
    centered(8, "Welcome back", CYAN_ON_BLUE);
    fill_rect(21, 11, 38, 5, ' ', PANEL_ATTR);
    text_at(24, 12, "User name:", PANEL_ATTR);
    text_at(24, 13, "> ", PANEL_ATTR);
    text_at(26, 13, input, PANEL_ATTR);
    centered(18, "Press Enter to start a local demo session", WHITE_ON_BLUE);
    centered(20, "Prototype sign-in: no password authentication yet", GREEN_ON_BLUE);
    centered(23, "Esc returns to setup", WHITE_ON_BLUE);
}

static void draw_wallpaper(void) {
    fill_rect(0, 0, VGA_WIDTH, VGA_HEIGHT, ' ', WHITE_ON_BLUE);
    /* A simple text-mode night-sky wallpaper with a bright horizon. */
    const size_t stars[][2] = {{4,3},{13,6},{24,2},{36,4},{48,3},{61,6},{74,2},
                               {9,10},{29,8},{53,9},{69,11},{18,13},{43,12}};
    for (size_t i = 0; i < sizeof(stars) / sizeof(stars[0]); ++i)
        cell(stars[i][0], stars[i][1], '.', CYAN_ON_BLUE);
    text_at(3, 5, "ENPY", YELLOW_ON_BLUE);
    text_at(3, 6, "[OS]", YELLOW_ON_BLUE);
    text_at(3, 9, "FILES", WHITE_ON_BLUE);
    text_at(3, 11, "[>_]", GREEN_ON_BLUE);
    text_at(3, 14, "TERMINAL", WHITE_ON_BLUE);
    text_at(3, 16, "[i]", CYAN_ON_BLUE);
    text_at(3, 19, "ABOUT", WHITE_ON_BLUE);

    /* Horizon / hills made from colored text-mode blocks. */
    for (size_t x = 0; x < VGA_WIDTH; ++x) {
        size_t ridge = 15 + ((x / 8) % 3);
        for (size_t y = ridge; y < 22; ++y)
            cell(x, y, (y == ridge) ? '^' : ' ', (y == ridge) ? GREEN_ON_BLUE : ATTR(2, 2));
    }
    fill_rect(0, 22, VGA_WIDTH, 2, ' ', ATTR(2, 15));
}

static void draw_app(void) {
    if (active_app == APP_NONE) return;
    fill_rect(19, 5, 57, 15, ' ', PANEL_ATTR);
    fill_rect(19, 5, 57, 1, ' ', ATTR(1, 15));
    if (active_app == APP_FILES) {
        text_at(21, 5, "File Browser - demo", ATTR(1, 15));
        text_at(22, 8, "This prototype has no disk/filesystem driver yet.", PANEL_ATTR);
        text_at(22, 10, "Your files are not available in this build.", PANEL_ATTR);
    } else if (active_app == APP_TERMINAL) {
        text_at(21, 5, "Terminal - demo", ATTR(1, 15));
        text_at(22, 8, "Type commands in the bottom bar and press Enter.", PANEL_ATTR);
        text_at(22, 10, "Commands: help, about, clear, echo TEXT, logout", PANEL_ATTR);
    } else if (active_app == APP_ABOUT) {
        text_at(21, 5, "About ENPY OS", ATTR(1, 15));
        text_at(22, 8, "ENPY OS is a freestanding 32-bit x86 hobby OS.", PANEL_ATTR);
        text_at(22, 10, "Text VGA desktop prototype; not Windows.", PANEL_ATTR);
        text_at(22, 12, "Graphics, networking, and app support are future work.", PANEL_ATTR);
    }
    text_at(22, 17, "Press Esc to close this window", PANEL_ATTR);
}

static void draw_desktop(void) {
    draw_wallpaper();
    fill_rect(0, 0, VGA_WIDTH, 1, ' ', ATTR(1, 15));
    text_at(2, 0, "ENPY OS", ATTR(1, 15));
    text_at(62, 0, "Desktop", ATTR(1, 15));
    text_at(13, 4, "Welcome,", WHITE_ON_BLUE);
    text_at(22, 4, username, YELLOW_ON_BLUE);
    text_at(13, 6, "1 Files   2 Terminal   3 About", WHITE_ON_BLUE);
    draw_app();

    fill_rect(0, 24, VGA_WIDTH, 1, PANEL_ATTR);
    text_at(1, 24, "[ENPY]", ATTR(1, 15));
    text_at(11, 24, "1 Files  2 Terminal  3 About  F1 Start  Esc close", PANEL_ATTR);

    fill_rect(0, 22, VGA_WIDTH, 2, ATTR(0, 15));
    text_at(1, 22, command_message ? "ENPY> " : "", WHITE_ON_DARK);
    text_at(7, 22, input, WHITE_ON_DARK);
    text_at(1, 23, command_message ? command_message : "Type help and press Enter for commands.", WHITE_ON_DARK);
}

static void set_message(const char *message) {
    command_message = 1;
    /* Keep output in a dedicated line and render it below the command entry. */
    size_t x = 1;
    while (*message && x < VGA_WIDTH) cell(x++, 23, *message++, WHITE_ON_DARK);
}

static void run_command(void) {
    input[input_length] = '\0';
    command_message = 0;
    if (equals(input, "help")) {
        draw_desktop();
        set_message("Commands: help, about, clear, echo TEXT, logout");
    } else if (equals(input, "about")) {
        draw_desktop();
        set_message("ENPY OS - 32-bit x86 hobby operating system prototype.");
    } else if (equals(input, "clear")) {
        draw_desktop();
    } else if (starts_with(input, "echo ")) {
        draw_desktop();
        set_message(input + 5);
    } else if (equals(input, "logout")) {
        input_length = 0;
        input[0] = '\0';
        screen = SCREEN_LOGIN;
        draw_login();
        return;
    } else if (input_length != 0) {
        draw_desktop();
        set_message("Unknown command. Type help.");
    } else {
        draw_desktop();
    }
    input_length = 0;
    input[0] = '\0';
    /* Preserve the result line while clearing the input field. */
    fill_rect(1, 22, VGA_WIDTH - 1, 1, ' ', ATTR(0, 15));
    text_at(1, 22, "ENPY> ", WHITE_ON_DARK);
}

static char scancode_to_char(uint8_t code, int shifted) {
    static const char normal[128] = {
        [0x02]='1',[0x03]='2',[0x04]='3',[0x05]='4',[0x06]='5',[0x07]='6',
        [0x08]='7',[0x09]='8',[0x0A]='9',[0x0B]='0',[0x0C]='-',[0x0D]='=',
        [0x0E]='\b',[0x10]='q',[0x11]='w',[0x12]='e',[0x13]='r',[0x14]='t',
        [0x15]='y',[0x16]='u',[0x17]='i',[0x18]='o',[0x19]='p',[0x1A]='[',
        [0x1B]=']',[0x1C]='\n',[0x1E]='a',[0x1F]='s',[0x20]='d',[0x21]='f',
        [0x22]='g',[0x23]='h',[0x24]='j',[0x25]='k',[0x26]='l',[0x27]=';',
        [0x28]='\'',[0x29]='`',[0x2B]='\\',[0x2C]='z',[0x2D]='x',[0x2E]='c',
        [0x2F]='v',[0x30]='b',[0x31]='n',[0x32]='m',[0x33]=',[',[0x34]='.',
        [0x35]='/',[0x39]=' '
    };
    static const char upper[128] = {
        [0x02]='!',[0x03]='@',[0x04]='#',[0x05]='$',[0x06]='%',[0x07]='^',
        [0x08]='&',[0x09]='*',[0x0A]='(',[0x0B]=')',[0x0C]='_',[0x0D]='+',
        [0x0E]='\b',[0x10]='Q',[0x11]='W',[0x12]='E',[0x13]='R',[0x14]='T',
        [0x15]='Y',[0x16]='U',[0x17]='I',[0x18]='O',[0x19]='P',[0x1A]='{',
        [0x1B]='}',[0x1C]='\n',[0x1E]='A',[0x1F]='S',[0x20]='D',[0x21]='F',
        [0x22]='G',[0x23]='H',[0x24]='J',[0x25]='K',[0x26]='L',[0x27]=':',
        [0x28]='"',[0x29]='~',[0x2B]='|',[0x2C]='Z',[0x2D]='X',[0x2E]='C',
        [0x2F]='V',[0x30]='B',[0x31]='N',[0x32]='M',[0x33]='<',[0x34]='>',
        [0x35]='?',[0x39]=' '
    };
    return shifted ? upper[code] : normal[code];
}

static void accept_username(void) {
    if (input_length == 0) {
        input[0] = 'G'; input[1] = 'u'; input[2] = 'e'; input[3] = 's';
        input[4] = 't'; input[5] = '\0'; input_length = 5;
    }
    size_t i = 0;
    while (i + 1 < USER_CAPACITY && input[i]) { username[i] = input[i]; ++i; }
    username[i] = '\0';
    input_length = 0;
    input[0] = '\0';
    screen = SCREEN_DESKTOP;
    active_app = APP_NONE;
    draw_desktop();
}

void keyboard_scancode_handler(uint8_t code) {
    if (code == 0x2A || code == 0x36) { shift_pressed = 1; return; }
    if (code == 0xAA || code == 0xB6) { shift_pressed = 0; return; }
    if (code & 0x80) return;

    if (screen == SCREEN_SETUP) {
        if (code == 0x02) { screen = SCREEN_INSTALL; draw_install(); }
        else if (code == 0x03) { screen = SCREEN_LOGIN; input_length = 0; input[0] = '\0'; draw_login(); }
        return;
    }
    if (screen == SCREEN_INSTALL) {
        if (code == 0x01) { screen = SCREEN_SETUP; draw_setup(); }
        else if (code == 0x1C) { screen = SCREEN_LOGIN; input_length = 0; input[0] = '\0'; draw_login(); }
        return;
    }
    if (screen == SCREEN_LOGIN) {
        if (code == 0x01) { screen = SCREEN_SETUP; input_length = 0; input[0] = '\0'; draw_setup(); return; }
        char c = scancode_to_char(code, shift_pressed);
        if (c == '\n') { accept_username(); return; }
        if (c == '\b') {
            if (input_length) input[--input_length] = '\0';
            draw_login();
        } else if (c >= ' ' && c <= '~' && input_length < USER_CAPACITY - 1) {
            input[input_length++] = c; input[input_length] = '\0'; draw_login();
        }
        return;
    }

    if (code == 0x01) {
        if (active_app != APP_NONE) { active_app = APP_NONE; draw_desktop(); }
        else { screen = SCREEN_LOGIN; input_length = 0; input[0] = '\0'; draw_login(); }
        return;
    }
    if (code == 0x3B) { active_app = APP_NONE; draw_desktop(); return; }
    if (code == 0x02) { active_app = APP_FILES; draw_desktop(); return; }
    if (code == 0x03) { active_app = APP_TERMINAL; draw_desktop(); return; }
    if (code == 0x04) { active_app = APP_ABOUT; draw_desktop(); return; }

    char c = scancode_to_char(code, shift_pressed);
    if (c == '\n') {
        run_command();
    } else if (c == '\b') {
        if (input_length) input[--input_length] = '\0';
        draw_desktop();
    } else if (c >= ' ' && c <= '~' && input_length < INPUT_CAPACITY - 1) {
        input[input_length++] = c; input[input_length] = '\0'; draw_desktop();
    }
}

void kernel_main(void) {
    disable_interrupts();
    input_length = 0;
    input[0] = '\0';
    screen = SCREEN_SETUP;
    init_idt();
    init_keyboard();
    draw_setup();
    enable_interrupts();
    for (;;) __asm__ volatile ("hlt");
}
