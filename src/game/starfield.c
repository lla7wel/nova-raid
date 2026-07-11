#include "game/game.h"

#define STARS 48

typedef struct { int16_t x; int32_t y; uint8_t layer; } star_t;
static star_t stars[STARS];

// Three parallax layers; layer 0 is slowest and dimmest.
static const int32_t layer_speed[3] = { FP(0.35), FP(0.8), FP(1.6) };
static const uint16_t layer_color[3] = {
    RGB565(70, 70, 100), RGB565(130, 130, 170), RGB565(220, 220, 255)
};

void starfield_init(void) {
    for (int i = 0; i < STARS; i++) {
        stars[i].x = hal_rand() % FB_W;
        stars[i].y = FP((int)(hal_rand() % FB_H));
        stars[i].layer = i % 3;
    }
}

void starfield_update(int speed_pct) {
    for (int i = 0; i < STARS; i++) {
        star_t *s = &stars[i];
        s->y += layer_speed[s->layer] * speed_pct / 100;
        if (s->y >= FP(FB_H)) {
            s->y -= FP(FB_H);
            s->x = hal_rand() % FB_W;
        }
    }
}

void starfield_render(void) {
    for (int i = 0; i < STARS; i++) {
        star_t *s = &stars[i];
        gfx_pixel(s->x, FP_INT(s->y), layer_color[s->layer]);
        if (s->layer == 2)
            gfx_pixel(s->x, FP_INT(s->y) - 1, layer_color[1]);
    }
}
