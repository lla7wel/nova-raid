#include "hal/hal.h"
#include "config.h"

#include "pico/stdlib.h"
#include "pico/rand.h"
#include "pico/multicore.h"
#include "hardware/adc.h"
#include "hardware/pwm.h"
#include "hardware/pio.h"
#include "hardware/flash.h"
#include "hardware/sync.h"
#include "ws2812.pio.h"

#include <string.h>

void hal_pico_init(void) {
    adc_init();
    adc_gpio_init(PIN_JOY_X);
    adc_gpio_init(PIN_JOY_Y);

    gpio_init(PIN_BTN_FIRE);
    gpio_set_dir(PIN_BTN_FIRE, GPIO_IN);
    gpio_pull_up(PIN_BTN_FIRE);
    gpio_init(PIN_BTN_ALT);
    gpio_set_dir(PIN_BTN_ALT, GPIO_IN);
    gpio_pull_up(PIN_BTN_ALT);

    gpio_init(PIN_LED_D1);
    gpio_set_dir(PIN_LED_D1, GPIO_OUT);
    gpio_init(PIN_LED_D2);
    gpio_set_dir(PIN_LED_D2, GPIO_OUT);

    gpio_set_function(PIN_BUZZER, GPIO_FUNC_PWM);

    PIO pio = pio0;
    uint offset = pio_add_program(pio, &ws2812_program);
    ws2812_program_init(pio, 0, offset, PIN_RGB, 800000, false);
    hal_rgb(0, 0, 0);
}

// --- Input ---

static uint16_t adc_read_ch(uint ch) {
    adc_select_input(ch);
    return adc_read();
}

void hal_input_raw(uint16_t *x, uint16_t *y) {
    *x = adc_read_ch(ADC_JOY_X);
    *y = adc_read_ch(ADC_JOY_Y);
}

static int8_t axis_scale(int32_t raw) {
    int32_t v = raw - 2048;
    if (v > -JOY_DEADZONE && v < JOY_DEADZONE) return 0;
    v += (v < 0) ? JOY_DEADZONE : -JOY_DEADZONE;
    v = v * 127 / (2048 - JOY_DEADZONE);
    if (v > 127) v = 127;
    if (v < -127) v = -127;
    return (int8_t)v;
}

void hal_input_read(input_t *in) {
    static bool prev_fire, prev_alt;

    uint16_t rx, ry;
    hal_input_raw(&rx, &ry);
#if JOY_SWAP_AXES
    uint16_t t = rx; rx = ry; ry = t;
#endif
    int8_t jx = axis_scale(rx);
    int8_t jy = axis_scale(ry);
#if JOY_INVERT_X
    jx = -jx;
#endif
#if JOY_INVERT_Y
    jy = -jy;
#endif
    in->jx = jx;
    in->jy = jy;

    in->fire = !gpio_get(PIN_BTN_FIRE);
    in->alt  = !gpio_get(PIN_BTN_ALT);
    in->fire_edge = in->fire && !prev_fire;
    in->alt_edge  = in->alt && !prev_alt;
    prev_fire = in->fire;
    prev_alt  = in->alt;
}

// --- Time / random ---

uint32_t hal_millis(void) {
    return to_ms_since_boot(get_absolute_time());
}

uint32_t hal_rand(void) {
    return get_rand_32();
}

// --- Buzzer ---

void hal_tone(uint16_t freq_hz, uint8_t vol) {
    uint slice = pwm_gpio_to_slice_num(PIN_BUZZER);
    if (freq_hz == 0 || vol == 0) {
        pwm_set_enabled(slice, false);
        return;
    }
    // 125 MHz / 64 = 1.953 MHz counter; wrap sets the pitch. Volume is
    // approximated by narrowing the duty cycle (piezo gets quieter).
    uint32_t wrap = 1953125u / freq_hz;
    if (wrap < 16) wrap = 16;
    if (wrap > 65535) wrap = 65535;
    pwm_set_clkdiv_int_frac(slice, 64, 0);
    pwm_set_wrap(slice, (uint16_t)wrap);
    uint32_t duty = (wrap / 2u) * vol / 255u;
    if (duty == 0) duty = 1;
    pwm_set_chan_level(slice, pwm_gpio_to_channel(PIN_BUZZER), (uint16_t)duty);
    pwm_set_enabled(slice, true);
}

// --- WS2812 ---

void hal_rgb(uint8_t r, uint8_t g, uint8_t b) {
    r = (uint8_t)((uint32_t)r * RGB_MAX_BRIGHT / 255u);
    g = (uint8_t)((uint32_t)g * RGB_MAX_BRIGHT / 255u);
    b = (uint8_t)((uint32_t)b * RGB_MAX_BRIGHT / 255u);
    uint32_t grb = ((uint32_t)g << 16) | ((uint32_t)r << 8) | b;
    pio_sm_put_blocking(pio0, 0, grb << 8u);
}

// --- Panel LEDs ---

void hal_led(int which, bool on) {
    gpio_put(which == 0 ? PIN_LED_D1 : PIN_LED_D2, on);
}

// --- Flash persistence ---
// The record lives in the last 4 KiB sector of the 2 MiB flash. Writes
// stop core 1 (multicore lockout) and disable IRQs because the RP2040
// cannot execute from flash while it is being programmed.

#define STORE_OFFSET (PICO_FLASH_SIZE_BYTES - FLASH_SECTOR_SIZE)

bool hal_store_load(void *buf, uint32_t len) {
    const uint8_t *p = (const uint8_t *)(XIP_BASE + STORE_OFFSET);
    uint32_t magic;
    memcpy(&magic, p, 4);
    if (magic != HISCORE_MAGIC) return false;
    memcpy(buf, p + 4, len);
    return true;
}

bool hal_store_save(const void *buf, uint32_t len) {
    if (len > FLASH_PAGE_SIZE - 4) return false;

    uint8_t page[FLASH_PAGE_SIZE];
    memset(page, 0xFF, sizeof page);
    uint32_t magic = HISCORE_MAGIC;
    memcpy(page, &magic, 4);
    memcpy(page + 4, buf, len);

    multicore_lockout_start_blocking();
    uint32_t irq = save_and_disable_interrupts();
    flash_range_erase(STORE_OFFSET, FLASH_SECTOR_SIZE);
    flash_range_program(STORE_OFFSET, page, FLASH_PAGE_SIZE);
    restore_interrupts(irq);
    multicore_lockout_end_blocking();
    return true;
}
