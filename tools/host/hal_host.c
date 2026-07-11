// Host implementation of hal.h for the frame-capture harness. Inputs are
// driven by the scenario script in main.c; sound, LEDs and persistence
// are no-ops beyond what the scenarios need.

#include "hal/hal.h"

#include <string.h>

input_t host_input;
static input_t prev;
static uint32_t ms;
static uint32_t rng = 0x4E524149u;

void hal_input_read(input_t *in) {
    *in = host_input;
    in->fire_edge = host_input.fire && !prev.fire;
    in->alt_edge = host_input.alt && !prev.alt;
    prev = host_input;
    host_input.fire_edge = host_input.alt_edge = false;
}

void hal_input_raw(uint16_t *x, uint16_t *y) {
    *x = (uint16_t)(2048 + (int)host_input.jx * 15);
    *y = (uint16_t)(2048 - (int)host_input.jy * 15);
}

uint32_t hal_millis(void) { ms += 40; return ms; }

uint32_t hal_rand(void) {
    rng ^= rng << 13;
    rng ^= rng >> 17;
    rng ^= rng << 5;
    return rng;
}

void hal_tone(uint16_t f, uint8_t v) { (void)f; (void)v; }
void hal_rgb(uint8_t r, uint8_t g, uint8_t b) { (void)r; (void)g; (void)b; }
void hal_led(int w, bool on) { (void)w; (void)on; }

static uint8_t store[256];
static bool store_valid;

bool hal_store_load(void *buf, uint32_t len) {
    if (!store_valid) return false;
    memcpy(buf, store, len);
    return true;
}

bool hal_store_save(const void *buf, uint32_t len) {
    if (len > sizeof store) return false;
    memcpy(store, buf, len);
    store_valid = true;
    return true;
}
