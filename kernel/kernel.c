#include <stddef.h>
#include <stdint.h>
#include "interrupts.h"

#define LOGICAL_W 800
#define LOGICAL_H 600
#define COLOR(r,g,b) (((uint32_t)(r) << 16) | ((uint32_t)(g) << 8) | (uint32_t)(b))

struct multiboot_info_graphics {
    uint32_t flags, mem_lower, mem_upper, boot_device, cmdline, mods_count, mods_addr;
    uint8_t symbols[16];
    uint32_t mmap_length, mmap_addr, drives_length, drives_addr, config_table;
    uint32_t boot_loader_name, apm_table, vbe_control_info;
    uint16_t vbe_mode_info, vbe_mode, vbe_interface_seg, vbe_interface_off, vbe_interface_len;
    uint16_t padding;
    uint64_t framebuffer_addr;
    uint32_t framebuffer_pitch, framebuffer_width, framebuffer_height;
    uint8_t framebuffer_bpp, framebuffer_type;
    uint8_t color_info[6];
} __attribute__((packed));

static volatile uint8_t *framebuffer;
static uint32_t fb_width, fb_height, fb_pitch;
static uint8_t fb_bpp;
static int mouse_x = 400, mouse_y = 300;
static uint8_t last_buttons;
static int start_open;
static int active_window;
static int mouse_available;
static const uint32_t navy = COLOR(13, 27, 55);
static const uint32_t white = COLOR(245, 248, 255);
static const uint32_t muted = COLOR(175, 194, 220);
static const uint32_t cyan = COLOR(69, 220, 226);
static const uint32_t panel = COLOR(10, 17, 35);
static const uint32_t window_bg = COLOR(238, 243, 250);

static int sx(int x) { return x * (int)fb_width / LOGICAL_W; }
static int sy(int y) { return y * (int)fb_height / LOGICAL_H; }

static void pixel_physical(int x, int y, uint32_t color) {
    if (x < 0 || y < 0 || (uint32_t)x >= fb_width || (uint32_t)y >= fb_height) return;
    volatile uint8_t *p = framebuffer + (uint32_t)y * fb_pitch + (uint32_t)x * (fb_bpp / 8);
    p[0] = (uint8_t)color;
    p[1] = (uint8_t)(color >> 8);
    p[2] = (uint8_t)(color >> 16);
    if (fb_bpp == 32) p[3] = 0;
}

static void rect(int x, int y, int w, int h, uint32_t color) {
    int x0 = sx(x), x1 = sx(x + w), y0 = sy(y), y1 = sy(y + h);
    if (x0 < 0) x0 = 0; if (y0 < 0) y0 = 0;
    if (x1 > (int)fb_width) x1 = (int)fb_width;
    if (y1 > (int)fb_height) y1 = (int)fb_height;
    for (int py = y0; py < y1; ++py)
        for (int px = x0; px < x1; ++px) pixel_physical(px, py, color);
}

/* Compact 5x7 bitmap font, stored as five vertical columns per glyph. */
static const uint8_t letters[26][5] = {
 {0x7E,0x11,0x11,0x11,0x7E},{0x7F,0x49,0x49,0x49,0x36},{0x3E,0x41,0x41,0x41,0x22},
 {0x7F,0x41,0x41,0x22,0x1C},{0x7F,0x49,0x49,0x49,0x41},{0x7F,0x09,0x09,0x09,0x01},
 {0x3E,0x41,0x49,0x49,0x7A},{0x7F,0x08,0x08,0x08,0x7F},{0x00,0x41,0x7F,0x41,0x00},
 {0x20,0x40,0x41,0x3F,0x01},{0x7F,0x08,0x14,0x22,0x41},{0x7F,0x40,0x40,0x40,0x40},
 {0x7F,0x02,0x0C,0x02,0x7F},{0x7F,0x04,0x08,0x10,0x7F},{0x3E,0x41,0x41,0x41,0x3E},
 {0x7F,0x09,0x09,0x09,0x06},{0x3E,0x41,0x51,0x21,0x5E},{0x7F,0x09,0x19,0x29,0x46},
 {0x46,0x49,0x49,0x49,0x31},{0x01,0x01,0x7F,0x01,0x01},{0x3F,0x40,0x40,0x40,0x3F},
 {0x1F,0x20,0x40,0x20,0x1F},{0x7F,0x20,0x18,0x20,0x7F},{0x63,0x14,0x08,0x14,0x63},
 {0x07,0x08,0x70,0x08,0x07},{0x61,0x51,0x49,0x45,0x43}
};
static const uint8_t digits[10][5] = {
 {0x3E,0x51,0x49,0x45,0x3E},{0x00,0x42,0x7F,0x40,0x00},{0x42,0x61,0x51,0x49,0x46},
 {0x21,0x41,0x45,0x4B,0x31},{0x18,0x14,0x12,0x7F,0x10},{0x27,0x45,0x45,0x45,0x39},
 {0x3C,0x4A,0x49,0x49,0x30},{0x01,0x71,0x09,0x05,0x03},{0x36,0x49,0x49,0x49,0x36},
 {0x06,0x49,0x49,0x29,0x1E}
};

static void glyph(char ch, uint8_t out[5]) {
    if (ch >= 'a' && ch <= 'z') ch = (char)(ch - 'a' + 'A');
    if (ch >= 'A' && ch <= 'Z') { for (int i=0;i<5;++i) out[i]=letters[ch-'A'][i]; return; }
    if (ch >= '0' && ch <= '9') { for (int i=0;i<5;++i) out[i]=digits[ch-'0'][i]; return; }
    uint8_t g[5] = {0,0,0,0,0};
    switch (ch) {
    case '.': g[2]=0x60; break; case ',': g[2]=0x60; g[1]=0x40; break;
    case ':': g[2]=0x36; break; case '-': g[1]=g[2]=g[3]=0x08; break;
    case '_': g[0]=g[1]=g[2]=g[3]=g[4]=0x40; break;
    case '/': g[0]=0x60; g[1]=0x18; g[2]=0x06; g[3]=0x01; break;
    case '>': g[0]=0x41; g[1]=0x22; g[2]=0x14; g[3]=0x08; break;
    case '<': g[1]=0x08; g[2]=0x14; g[3]=0x22; g[4]=0x41; break;
    case '[': g[0]=0x7F; g[1]=0x41; g[2]=0x41; break;
    case ']': g[2]=0x41; g[3]=0x41; g[4]=0x7F; break;
    case '|': g[2]=0x7F; break; case '!': g[2]=0x5F; break;
    case '?': g[0]=0x02; g[1]=0x01; g[2]=0x51; g[3]=0x09; g[4]=0x06; break;
    case '+': g[1]=0x08; g[2]=0x3E; g[3]=0x08; break;
    case '=': g[1]=g[2]=g[3]=0x14; break;
    case ' ': break;
    default: g[0]=g[1]=g[2]=g[3]=g[4]=0x41; break;
    }
    for (int i=0;i<5;++i) out[i]=g[i];
}

static void text(int x, int y, const char *s, uint32_t color, int scale) {
    while (*s) {
        uint8_t g[5]; glyph(*s++, g);
        for (int cx=0;cx<5;++cx) for (int cy=0;cy<7;++cy)
            if (g[cx] & (1u << cy)) rect(x+cx*scale,y+cy*scale,scale,scale,color);
        x += 6 * scale;
    }
}

static void draw_wallpaper(void) {
    for (int y=0;y<LOGICAL_H;++y) {
        uint8_t r=(uint8_t)(8+y/34), g=(uint8_t)(25+y/15), b=(uint8_t)(54+y/9);
        rect(0,y,LOGICAL_W,1,COLOR(r,g,b));
    }
    /* Stylized sunrise and layered hills. */
    for (int y=255;y<410;++y) for (int x=500;x<655;++x) {
        int dx=x-578, dy=y-340;
        if (dx*dx+dy*dy < 73*73) pixel_physical(sx(x),sy(y),COLOR(255,190,103));
    }
    for (int x=0;x<LOGICAL_W;++x) {
        int ridge=390 + ((x*17/LOGICAL_W)%75) - 30;
        rect(x,ridge,1,LOGICAL_H-ridge,COLOR(28,92,104));
        int ridge2=455 + ((x*11/LOGICAL_W)%50) - 15;
        rect(x,ridge2,1,LOGICAL_H-ridge2,COLOR(17,58,75));
    }
}

static void draw_icon(int x,int y,const char *label,int kind) {
    rect(x+7,y+6,38,34,kind==0?COLOR(255,205,93):kind==1?COLOR(94,205,255):COLOR(145,232,177));
    rect(x+11,y+10,30,26,kind==0?COLOR(255,226,145):kind==1?COLOR(30,99,160):COLOR(36,125,99));
    text(x,y+47,label,white,1);
}

static void draw_window(void) {
    if (!active_window) return;
    rect(190,92,505,372,COLOR(0,0,0));
    rect(184,86,505,372,window_bg);
    rect(184,86,505,38,COLOR(28,54,91));
    text(202,99,active_window==1?"FILES":active_window==2?"TERMINAL":"ABOUT ENPY OS",white,2);
    rect(650,94,27,24,COLOR(190,58,73));
    text(659,99,"X",white,2);
    if (active_window==1) {
        text(211,151,"FILE EXPLORER",COLOR(25,49,78),2);
        rect(210,186,455,1,COLOR(190,202,216));
        text(211,210,"NO DISK DRIVER IS AVAILABLE IN THIS BUILD.",COLOR(45,64,87),1);
        text(211,242,"This window is a desktop UI prototype.",COLOR(45,64,87),1);
    } else if (active_window==2) {
        rect(202,137,469,305,COLOR(17,24,39));
        text(220,158,"ENPY TERMINAL - UI DEMO",COLOR(132,240,191),2);
        text(220,201,"A shell and process system are not implemented yet.",white,1);
        text(220,229,"Click the close button to return to the desktop.",muted,1);
    } else {
        text(211,154,"ENPY OS",COLOR(28,65,112),3);
        text(211,211,"A freestanding 32-bit x86 hobby operating system.",COLOR(45,64,87),1);
        text(211,240,"Graphical framebuffer desktop prototype.",COLOR(45,64,87),1);
        text(211,269,"No filesystem, network, or Windows app support yet.",COLOR(45,64,87),1);
    }
}

static void draw_start_menu(void) {
    if (!start_open) return;
    rect(14,330,285,208,COLOR(0,0,0));
    rect(10,324,285,208,COLOR(239,244,251));
    rect(10,324,285,48,COLOR(28,54,91));
    text(28,339,"ENPY OS",white,2);
    rect(28,389,240,36,COLOR(221,231,243)); text(42,400,"FILES",COLOR(25,49,78),2);
    rect(28,432,240,36,COLOR(221,231,243)); text(42,443,"TERMINAL",COLOR(25,49,78),2);
    rect(28,475,240,36,COLOR(221,231,243)); text(42,486,"ABOUT",COLOR(25,49,78),2);
}

static void draw_taskbar(void) {
    rect(0,550,LOGICAL_W,50,panel);
    rect(0,550,LOGICAL_W,1,COLOR(67,92,124));
    rect(14,558,106,34,COLOR(29,93,145));
    text(29,568,"START",white,2);
    rect(132,558,40,34,COLOR(30,46,69)); text(145,568,"H",cyan,2);
    if (active_window) {
        rect(181,558,138,34,COLOR(39,59,86));
        text(194,568,active_window==1?"FILES":active_window==2?"TERM":"ABOUT",white,1);
    }
    text(700,568,"ENPY",muted,1);
}

static void draw_pointer(void) {
    int x=mouse_x, y=mouse_y;
    for (int row=0;row<16;++row) {
        int width= row<11 ? row/2+1 : 7-(row-11)/2;
        for (int col=0;col<width;++col) {
            rect(x+col,y+row,1,1,col==0||row==0?COLOR(8,15,28):white);
        }
    }
}

static void draw_desktop(void) {
    draw_wallpaper();
    rect(0,0,LOGICAL_W,42,COLOR(8,17,34));
    text(20,14,"ENPY OS",white,2);
    text(165,17,"DESKTOP",muted,1);
    draw_icon(28,78,"FILES",0);
    draw_icon(28,166,"TERMINAL",1);
    draw_icon(28,254,"ABOUT",2);
    text(170,380,"WELCOME TO ENPY OS",white,3);
    text(172,420,"A fresh desktop prototype",muted,2);
    draw_window();
    draw_taskbar();
    draw_start_menu();
    draw_pointer();
}

static void open_app(int app) { active_window=app; start_open=0; draw_desktop(); }

static void handle_click(int x,int y) {
    if (start_open) {
        if (x>=20 && x<285 && y>=385 && y<425) open_app(1);
        else if (x>=20 && x<285 && y>=428 && y<470) open_app(2);
        else if (x>=20 && x<285 && y>=472 && y<515) open_app(3);
        else { start_open=0; draw_desktop(); }
        return;
    }
    if (y>=550 && x<125) { start_open=1; draw_desktop(); return; }
    if (active_window && x>=645 && x<685 && y>=86 && y<128) {
        active_window=0; draw_desktop(); return;
    }
    if (active_window) return;
    if (x>=20 && x<115 && y>=75 && y<151) open_app(1);
    else if (x>=20 && x<125 && y>=163 && y<241) open_app(2);
    else if (x>=20 && x<115 && y>=251 && y<329) open_app(3);
    else if (y>=550 && x>=132 && x<175) { active_window=2; draw_desktop(); }
}

static void show_boot_error(const char *message) {
    volatile uint16_t *vga=(volatile uint16_t *)0xB8000;
    for (size_t i=0;i<80*25;++i) vga[i]=0x4F20;
    for (size_t i=0;message[i] && i<80;++i) vga[2*80+2+i]=(uint16_t)0x4F00|(uint8_t)message[i];
}

void keyboard_scancode_handler(uint8_t code) {
    if (code & 0x80) return;
    if (code == 0x01) { active_window=0; start_open=0; draw_desktop(); }
    else if (code == 0x3B) { start_open=!start_open; draw_desktop(); }
}

void kernel_main(uint32_t multiboot_address) {
    disable_interrupts();
    init_idt();
    init_keyboard();
    const struct multiboot_info_graphics *mbi =
        (const struct multiboot_info_graphics *)(uintptr_t)multiboot_address;
    if (!mbi || !(mbi->flags & (1u<<12)) || mbi->framebuffer_addr > 0xFFFFFFFFu ||
        mbi->framebuffer_width < 640 || mbi->framebuffer_height < 480 ||
        (mbi->framebuffer_bpp != 24 && mbi->framebuffer_bpp != 32) ||
        mbi->framebuffer_type != 1) {
        show_boot_error("ENPY OS: GRUB did not provide a supported RGB framebuffer.");
        for (;;) __asm__ volatile ("hlt");
    }
    framebuffer=(volatile uint8_t *)(uintptr_t)(uint32_t)mbi->framebuffer_addr;
    fb_width=mbi->framebuffer_width;
    fb_height=mbi->framebuffer_height;
    fb_pitch=mbi->framebuffer_pitch;
    fb_bpp=mbi->framebuffer_bpp;
    mouse_x=(int)fb_width/2;
    mouse_y=(int)fb_height/2;
    init_mouse();
    draw_desktop();

    for (;;) {
        int dx=0,dy=0;
        uint8_t buttons=last_buttons;
        if (mouse_poll(&dx,&dy,&buttons)) {
            mouse_x+=dx; mouse_y+=dy;
            if (mouse_x<0) mouse_x=0;
            if (mouse_y<0) mouse_y=0;
            if (mouse_x>=(int)fb_width) mouse_x=(int)fb_width-1;
            if (mouse_y>=(int)fb_height) mouse_y=(int)fb_height-1;
            int logical_x=mouse_x*LOGICAL_W/(int)fb_width;
            int logical_y=mouse_y*LOGICAL_H/(int)fb_height;
            if ((buttons&1) && !(last_buttons&1)) handle_click(logical_x,logical_y);
            else draw_desktop();
            last_buttons=buttons;
        }
        (void)keyboard_poll();
        __asm__ volatile ("pause");
    }
}
