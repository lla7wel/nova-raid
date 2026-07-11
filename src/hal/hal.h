#ifndef NOVA_HAL_H
#define NOVA_HAL_H

// Platform interface consumed by the game core. Implemented by
// hal/hal_pico.c on hardware and tools/host/hal_host.c for the
// frame-capture harness, keeping game logic free of Pico headers.

#include <stdint.h>
#include <stdbool.h>

typedef struct {
    // Joystick, deadzone applied, -128..127 (right/up positive).
    int8_t jx, jy;
    // Current level and rising edge for both buttons.
    bool fire, alt;
    bool fire_edge, alt_edge;
} input_t;

void     hal_input_read(input_t *in);
// Raw ADC counts (0..4095), used only by the diagnostics screen.
void     hal_input_raw(uint16_t *x, uint16_t *y);

uint32_t hal_millis(void);
uint32_t hal_rand(void);

// Buzzer: freq_hz 0 silences. vol 0..255 maps to PWM duty.
void     hal_tone(uint16_t freq_hz, uint8_t vol);

// WS2812, gamma/brightness handled inside the HAL.
void     hal_rgb(uint8_t r, uint8_t g, uint8_t b);

// Panel LEDs D1 (bomb ready) and D2 (low health).
void     hal_led(int which, bool on);

// Returns false if no valid record exists (blank flash / first boot).
bool     hal_store_load(void *buf, uint32_t len);
bool     hal_store_save(const void *buf, uint32_t len);

#endif
