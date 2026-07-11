#ifndef ST7796_H
#define ST7796_H

#include <stdint.h>

// Panel driver for the kit's 3.5" 480x320 TFT (ST7796S controller, SPI).
// The panel is always addressed in landscape (480x320); st7796_push_frame
// pixel-doubles a 240x160 native-RGB565 framebuffer while streaming it.

void st7796_init(void);
void st7796_push_frame(const uint16_t *fb);
void st7796_fill(uint16_t rgb565);

#endif
