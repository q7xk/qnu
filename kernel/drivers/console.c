#include <qnu/console.h>

static u32 *fb;
static u32 fb_w, fb_h, fb_pitch_pixels;
static u32 cur_x, cur_y;
static u32 fg = 0xE6EDF3;
static u32 bg = 0x0B0F14;
static const u32 CHAR_W = 8;
static const u32 CHAR_H = 16;

/* 8x16 bitmap font for ASCII 32-126, 16 bytes per glyph */
static const u8 font[95][16] = {
#include "font8x16.inc"
};

static void draw_char(char c, u32 x, u32 y) {
    if (c < 32 || c > 126) c = '?';
    const u8 *g = font[(int)c - 32];
    for (u32 row = 0; row < CHAR_H; row++) {
        u8 bits = g[row];
        for (u32 col = 0; col < CHAR_W; col++) {
            u32 color = (bits & (0x80 >> col)) ? fg : bg;
            u32 px = x + col;
            u32 py = y + row;
            if (px < fb_w && py < fb_h) fb[py * fb_pitch_pixels + px] = color;
        }
    }
}

void console_clear(void) {
    if (!fb) return;
    for (u32 y = 0; y < fb_h; y++)
        for (u32 x = 0; x < fb_w; x++)
            fb[y * fb_pitch_pixels + x] = bg;
    cur_x = 0; cur_y = 0;
}

static void scroll(void) {
    if (!fb) return;
    u32 row_pixels = CHAR_H * fb_pitch_pixels;
    u32 screen_rows = fb_h / CHAR_H;
    if (screen_rows == 0) return;
    /* move up one text row */
    for (u32 y = 0; y < (screen_rows - 1) * CHAR_H; y++) {
        for (u32 x = 0; x < fb_w; x++) {
            fb[y * fb_pitch_pixels + x] = fb[(y + CHAR_H) * fb_pitch_pixels + x];
        }
    }
    /* clear last row */
    for (u32 y = (screen_rows - 1) * CHAR_H; y < fb_h; y++) {
        for (u32 x = 0; x < fb_w; x++) {
            fb[y * fb_pitch_pixels + x] = bg;
        }
    }
    if (cur_y >= CHAR_H) cur_y -= CHAR_H;
}

void console_init(void *fb_addr, u32 width, u32 height, u32 pitch, u32 bpp) {
    if (bpp != 32) { fb = NULL; return; }
    fb = (u32 *)fb_addr;
    fb_w = width;
    fb_h = height;
    fb_pitch_pixels = pitch / 4;
    cur_x = 0; cur_y = 0;
    console_clear();
}

void console_putc(char c) {
    if (!fb) return;
    if (c == '\n') { cur_x = 0; cur_y += CHAR_H; }
    else if (c == '\r') { cur_x = 0; }
    else {
        draw_char(c, cur_x, cur_y);
        cur_x += CHAR_W;
        if (cur_x + CHAR_W > fb_w) { cur_x = 0; cur_y += CHAR_H; }
    }
    if (cur_y + CHAR_H > fb_h) scroll();
}

void console_write(const char *s) {
    while (*s) console_putc(*s++);
}
