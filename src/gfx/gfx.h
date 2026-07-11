#ifndef NOVA_GFX_H
#define NOVA_GFX_H

#include <stdint.h>
#include <stdbool.h>
#include "config.h"

// All drawing targets a 240x160 native-RGB565 framebuffer selected with
// gfx_set_target. Coordinates are clipped, so callers may draw partially
// (or fully) off screen.

#define RGB565(r, g, b) \
    ((uint16_t)((((r) & 0xF8) << 8) | (((g) & 0xFC) << 3) | ((b) >> 3)))

enum {
    C_BLACK   = RGB565(0, 0, 0),
    C_WHITE   = RGB565(255, 255, 255),
    C_RED     = RGB565(255, 48, 48),
    C_ORANGE  = RGB565(255, 150, 32),
    C_YELLOW  = RGB565(255, 220, 64),
    C_GREEN   = RGB565(64, 255, 96),
    C_CYAN    = RGB565(64, 224, 255),
    C_BLUE    = RGB565(64, 96, 255),
    C_MAGENTA = RGB565(230, 64, 255),
    C_GREY    = RGB565(128, 128, 140),
    C_DGREY   = RGB565(56, 56, 68),
    C_DBLUE   = RGB565(16, 20, 56),
    C_PURPLE  = RGB565(120, 48, 200),
};

typedef struct {
    uint8_t w, h;
    const uint8_t *px; // palette indices into sprite_palette, 0 transparent
} sprite_t;

void gfx_set_target(uint16_t *fb);
uint16_t *gfx_target(void);

void gfx_clear(uint16_t c);
void gfx_pixel(int x, int y, uint16_t c);
void gfx_hline(int x, int y, int w, uint16_t c);
void gfx_vline(int x, int y, int h, uint16_t c);
void gfx_rect(int x, int y, int w, int h, uint16_t c);
void gfx_fill_rect(int x, int y, int w, int h, uint16_t c);
// Halves the RGB of every pixel in the region (pause / dialog backdrop).
void gfx_dim_rect(int x, int y, int w, int h);

// 8x8 font, integer scale >= 1.
void gfx_char(int x, int y, char ch, uint16_t c, int scale);
void gfx_text(int x, int y, const char *s, uint16_t c, int scale);
void gfx_text_center(int y, const char *s, uint16_t c, int scale);
int  gfx_text_width(const char *s, int scale);

void gfx_sprite(int x, int y, const sprite_t *s);
void gfx_sprite_flip(int x, int y, const sprite_t *s, bool flip_y);
// Draws every opaque sprite pixel in a flat color (hit flash).
void gfx_sprite_tint(int x, int y, const sprite_t *s, uint16_t c);

#endif
