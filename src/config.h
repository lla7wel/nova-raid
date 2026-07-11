#ifndef NOVA_CONFIG_H
#define NOVA_CONFIG_H

#define NOVA_VERSION "1.0.1"

// Hardware configuration for the 52Pi / GeeekPi Pico Breadboard Kit Plus
// (EP-0172). Every pin below matches the kit's silk screen and the 52Pi
// wiki. If your board revision differs, this file is the only place that
// needs editing.

// --- 3.5" TFT, ST7796 controller, SPI0 ---
#define LCD_SPI            spi0
#define PIN_LCD_SCK        2
#define PIN_LCD_MOSI       3
#define PIN_LCD_MISO       4   // wired on the kit, unused by the game
#define PIN_LCD_CS         5
#define PIN_LCD_DC         6
#define PIN_LCD_RST        7

// 62.5 MHz is the fastest SPI0 clock the RP2040 produces from the default
// 125 MHz system clock and is within what this panel handles (the vendor
// demo drives it at 60 MHz). Lower it if you see a corrupted image.
#define LCD_BAUD_HZ        (62500u * 1000u)

// Panel rotation, 0..3. 1 and 3 are landscape (480x320). Default 1 is
// upright when the board is held with the USB cable on the LEFT (Pico
// strip left, joystick and buttons along the bottom) — verified on
// hardware. Prefer USB on the right? Set 3. The diagnostics arrow must
// point at the physical top edge.
#define LCD_ROTATION       1

// Capacitive touch (I2C0, GP8/GP9) is present on the kit but intentionally
// unused: every game input maps to the joystick and buttons.

// --- Inputs ---
#define PIN_JOY_X          26  // ADC0
#define PIN_JOY_Y          27  // ADC1
#define ADC_JOY_X          0
#define ADC_JOY_Y          1

// Set to 1 if pushing the stick right/up moves the ship the wrong way.
// Confirm with the Diagnostics screen; analog orientation could not be
// physically verified and may differ between board lots.
#define JOY_INVERT_X       0
#define JOY_INVERT_Y       1   // kit stick reports higher ADC when pulled down
#define JOY_SWAP_AXES      0

#define JOY_DEADZONE       300 // of +/-2048 around centre, absorbs drift

#define PIN_BTN_FIRE       15  // silk "BTN1"
#define PIN_BTN_ALT        14  // silk "BTN2" - nova bomb / back
// Buttons are wired active-low with external pull-ups on the kit; the
// firmware still enables internal pull-ups, which is harmless either way.

// --- Outputs ---
#define PIN_BUZZER         13  // passive piezo, PWM driven
#define PIN_RGB            12  // single WS2812
#define PIN_LED_D1         16  // panel LED D1: nova bomb ready
#define PIN_LED_D2         17  // panel LED D2: low-health warning

#define RGB_MAX_BRIGHT     48  // of 255; full WS2812 brightness is blinding

// --- Renderer ---
// The game renders a 240x160 RGB565 framebuffer that core 1 doubles to
// 480x320 while streaming it to the panel. Two buffers = 150 KiB of the
// RP2040's 264 KiB SRAM; do not raise the resolution without checking the
// memory map.
#define FB_W               240
#define FB_H               160
#define GAME_FPS           25  // full 480x320 frame takes ~39 ms on SPI

// --- Persistence ---
// High scores live in the last 4 KiB flash sector, far above any firmware
// this project produces. Erased/blank flash is detected by magic number.
#define HISCORE_MAGIC      0x4E525631u // "NRV1"

#endif
