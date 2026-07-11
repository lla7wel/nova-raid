#include "gfx/gfx.h"
#include "gfx/sprites.h"
#include "gfx/font8x8_basic.h"

#include <string.h>

static uint16_t *fb;

void gfx_set_target(uint16_t *target) { fb = target; }
uint16_t *gfx_target(void) { return fb; }

void gfx_clear(uint16_t c) {
    if ((c >> 8) == (c & 0xFF)) {
        memset(fb, c & 0xFF, FB_W * FB_H * 2);
        return;
    }
    for (int i = 0; i < FB_W * FB_H; i++) fb[i] = c;
}

void gfx_pixel(int x, int y, uint16_t c) {
    if ((unsigned)x >= FB_W || (unsigned)y >= FB_H) return;
    fb[y * FB_W + x] = c;
}

void gfx_hline(int x, int y, int w, uint16_t c) {
    if ((unsigned)y >= FB_H) return;
    if (x < 0) { w += x; x = 0; }
    if (x + w > FB_W) w = FB_W - x;
    uint16_t *p = fb + y * FB_W + x;
    for (int i = 0; i < w; i++) p[i] = c;
}

void gfx_vline(int x, int y, int h, uint16_t c) {
    if ((unsigned)x >= FB_W) return;
    if (y < 0) { h += y; y = 0; }
    if (y + h > FB_H) h = FB_H - y;
    uint16_t *p = fb + y * FB_W + x;
    for (int i = 0; i < h; i++, p += FB_W) *p = c;
}

void gfx_rect(int x, int y, int w, int h, uint16_t c) {
    gfx_hline(x, y, w, c);
    gfx_hline(x, y + h - 1, w, c);
    gfx_vline(x, y, h, c);
    gfx_vline(x + w - 1, y, h, c);
}

void gfx_fill_rect(int x, int y, int w, int h, uint16_t c) {
    if (x < 0) { w += x; x = 0; }
    if (y < 0) { h += y; y = 0; }
    if (x + w > FB_W) w = FB_W - x;
    if (y + h > FB_H) h = FB_H - y;
    if (w <= 0) return;
    for (int j = 0; j < h; j++) {
        uint16_t *p = fb + (y + j) * FB_W + x;
        for (int i = 0; i < w; i++) p[i] = c;
    }
}

void gfx_dim_rect(int x, int y, int w, int h) {
    if (x < 0) { w += x; x = 0; }
    if (y < 0) { h += y; y = 0; }
    if (x + w > FB_W) w = FB_W - x;
    if (y + h > FB_H) h = FB_H - y;
    if (w <= 0) return;
    for (int j = 0; j < h; j++) {
        uint16_t *p = fb + (y + j) * FB_W + x;
        for (int i = 0; i < w; i++)
            p[i] = (p[i] >> 1) & 0x7BEF; // halve each RGB565 channel
    }
}

void gfx_char(int x, int y, char ch, uint16_t c, int scale) {
    if (ch < 0x20 || ch > 0x7E) ch = '?';
    const unsigned char *glyph = (const unsigned char *)font8x8_basic[(int)ch];
    for (int row = 0; row < 8; row++) {
        unsigned char bits = glyph[row];
        for (int col = 0; col < 8; col++) {
            if (!(bits & (1u << col))) continue;
            if (scale == 1)
                gfx_pixel(x + col, y + row, c);
            else
                gfx_fill_rect(x + col * scale, y + row * scale, scale, scale, c);
        }
    }
}

void gfx_text(int x, int y, const char *s, uint16_t c, int scale) {
    for (; *s; s++, x += 8 * scale)
        gfx_char(x, y, *s, c, scale);
}

int gfx_text_width(const char *s, int scale) {
    return (int)strlen(s) * 8 * scale;
}

void gfx_text_center(int y, const char *s, uint16_t c, int scale) {
    gfx_text((FB_W - gfx_text_width(s, scale)) / 2, y, s, c, scale);
}

void gfx_sprite_flip(int x, int y, const sprite_t *s, bool flip_y) {
    for (int j = 0; j < s->h; j++) {
        int sy = flip_y ? s->h - 1 - j : j;
        const uint8_t *row = s->px + sy * s->w;
        for (int i = 0; i < s->w; i++) {
            uint8_t idx = row[i];
            if (idx) gfx_pixel(x + i, y + j, sprite_palette[idx]);
        }
    }
}

void gfx_sprite(int x, int y, const sprite_t *s) {
    gfx_sprite_flip(x, y, s, false);
}

void gfx_sprite_tint(int x, int y, const sprite_t *s, uint16_t c) {
    for (int j = 0; j < s->h; j++) {
        const uint8_t *row = s->px + j * s->w;
        for (int i = 0; i < s->w; i++)
            if (row[i]) gfx_pixel(x + i, y + j, c);
    }
}
