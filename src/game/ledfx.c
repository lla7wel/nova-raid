#include "game/game.h"

// WS2812 feedback: a persistent base mode (menu breathing, boss pulse...)
// with transient event overlays (damage flash, power-up blip) on top.

typedef struct { uint8_t r, g, b; } rgb_t;

static led_event_t base = LED_OFF;
static led_event_t event = LED_OFF;
static uint16_t event_t, base_t;

void ledfx_set(led_event_t b) {
    base = b;
    base_t = 0;
}

void ledfx_event(led_event_t ev) {
    event = ev;
    event_t = 0;
}

// Triangle wave 0..255..0 with the given period in frames.
static uint8_t tri(uint16_t t, uint16_t period) {
    uint32_t ph = (t % period) * 512 / period;
    return ph < 256 ? ph : 511 - ph;
}

static rgb_t scale(rgb_t c, uint8_t s) {
    return (rgb_t){ (uint8_t)(c.r * s / 255), (uint8_t)(c.g * s / 255),
                    (uint8_t)(c.b * s / 255) };
}

static rgb_t base_color(void) {
    switch (base) {
        case LED_MENU:
            return scale((rgb_t){0, 180, 255}, (uint8_t)(40 + tri(base_t, 100) / 2));
        case LED_PLAY:
            return (rgb_t){0, 10, 25};
        case LED_BOSS:
            return scale((rgb_t){255, 60, 0}, (uint8_t)(30 + tri(base_t, 30)));
        case LED_GAMEOVER:
            return scale((rgb_t){255, 0, 0}, (uint8_t)(20 + tri(base_t, 120) / 3));
        case LED_VICTORY: {
            // slow hue rotation across R->G->B
            uint8_t ph = (base_t * 4) & 0xFF;
            uint8_t r = tri(ph, 256), g = tri((ph + 85) & 0xFF, 256),
                    b = tri((ph + 170) & 0xFF, 256);
            return (rgb_t){r, g, b};
        }
        default:
            return (rgb_t){0, 0, 0};
    }
}

static bool event_color(rgb_t *out) {
    uint16_t t = event_t;
    switch (event) {
        case LED_DAMAGE:
            if (t > 12) return false;
            *out = scale((rgb_t){255, 0, 0}, (uint8_t)(255 - t * 20));
            return true;
        case LED_POWERUP:
            if (t > 10) return false;
            *out = scale((rgb_t){0, 255, 60}, (uint8_t)(255 - t * 24));
            return true;
        case LED_SHIELD:
            if (t > 10) return false;
            *out = scale((rgb_t){0, 90, 255}, (uint8_t)(255 - t * 24));
            return true;
        case LED_BOMB:
            if (t > 8) return false;
            *out = scale((rgb_t){255, 255, 255}, (uint8_t)(255 - t * 30));
            return true;
        case LED_LIFE:
            if (t > 20) return false;
            *out = scale((rgb_t){255, 60, 200}, tri(t * 12, 256));
            return true;
        default:
            return false;
    }
}

void ledfx_update(void) {
    base_t++;
    event_t++;
    rgb_t c;
    if (!event_color(&c)) {
        event = LED_OFF;
        c = base_color();
    }
    hal_rgb(c.r, c.g, c.b);
}
