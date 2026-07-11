#include "st7796.h"
#include "config.h"

#include "pico/stdlib.h"
#include "hardware/spi.h"
#include "hardware/dma.h"
#include "hardware/gpio.h"

#define LCD_W 480
#define LCD_H 320

static int dma_ch;
static dma_channel_config dma_cfg;

// Two line buffers so the CPU scales line N+1 while DMA streams line N.
// One panel line = 480 px, sent twice (vertical doubling).
static uint16_t linebuf[2][LCD_W];

static inline void cs(bool sel)  { gpio_put(PIN_LCD_CS, !sel); }
static inline void dc_data(bool d) { gpio_put(PIN_LCD_DC, d); }

static void cmd(uint8_t c) {
    dc_data(false);
    spi_write_blocking(LCD_SPI, &c, 1);
    dc_data(true);
}

static void data8(uint8_t d)  { spi_write_blocking(LCD_SPI, &d, 1); }

static void cmd_n(uint8_t c, const uint8_t *d, int n) {
    cmd(c);
    if (n) spi_write_blocking(LCD_SPI, d, n);
}

// MADCTL bits: MY 0x80, MX 0x40, MV 0x20, ML 0x10, BGR 0x08.
static uint8_t madctl_for_rotation(void) {
    switch (LCD_ROTATION & 3) {
        case 0:  return 0x48;              // portrait 320x480
        case 1:  return 0x28;              // landscape 480x320
        case 2:  return 0x88;              // portrait flipped
        default: return 0xE8;              // landscape flipped
    }
}

static void set_window(uint16_t x0, uint16_t y0, uint16_t x1, uint16_t y1) {
    uint8_t ca[] = { x0 >> 8, x0 & 0xFF, x1 >> 8, x1 & 0xFF };
    uint8_t ra[] = { y0 >> 8, y0 & 0xFF, y1 >> 8, y1 & 0xFF };
    cmd_n(0x2A, ca, 4);
    cmd_n(0x2B, ra, 4);
    cmd(0x2C);
}

void st7796_init(void) {
    spi_init(LCD_SPI, LCD_BAUD_HZ);
    gpio_set_function(PIN_LCD_SCK, GPIO_FUNC_SPI);
    gpio_set_function(PIN_LCD_MOSI, GPIO_FUNC_SPI);
    gpio_set_function(PIN_LCD_MISO, GPIO_FUNC_SPI);

    gpio_init(PIN_LCD_CS);
    gpio_init(PIN_LCD_DC);
    gpio_init(PIN_LCD_RST);
    gpio_set_dir(PIN_LCD_CS, GPIO_OUT);
    gpio_set_dir(PIN_LCD_DC, GPIO_OUT);
    gpio_set_dir(PIN_LCD_RST, GPIO_OUT);
    cs(false);

    gpio_put(PIN_LCD_RST, 1);
    sleep_ms(10);
    gpio_put(PIN_LCD_RST, 0);
    sleep_ms(10);
    gpio_put(PIN_LCD_RST, 1);
    sleep_ms(120);

    cs(true);

    cmd(0x01);            // software reset
    sleep_ms(120);
    cmd(0x11);            // sleep out
    sleep_ms(120);

    // Command set control: unlock the ST7796 extension commands used below.
    cmd_n(0xF0, (const uint8_t[]){0xC3}, 1);
    cmd_n(0xF0, (const uint8_t[]){0x96}, 1);

    cmd(0x36); data8(madctl_for_rotation());
    cmd(0x3A); data8(0x55); // 16-bit RGB565

    cmd_n(0xB4, (const uint8_t[]){0x01}, 1);                 // 1-dot inversion
    cmd_n(0xB6, (const uint8_t[]){0x80, 0x02, 0x3B}, 3);     // display function
    cmd_n(0xE8, (const uint8_t[]){0x40, 0x8A, 0x00, 0x00,
                                  0x29, 0x19, 0xA5, 0x33}, 8);
    cmd_n(0xC1, (const uint8_t[]){0x06}, 1);
    cmd_n(0xC2, (const uint8_t[]){0xA7}, 1);
    cmd_n(0xC5, (const uint8_t[]){0x18}, 1);
    sleep_ms(120);
    cmd_n(0xE0, (const uint8_t[]){0xF0, 0x09, 0x0B, 0x06, 0x04, 0x15,
                                  0x2F, 0x54, 0x42, 0x3C, 0x17, 0x14,
                                  0x18, 0x1B}, 14);
    cmd_n(0xE1, (const uint8_t[]){0xE0, 0x09, 0x0B, 0x06, 0x04, 0x03,
                                  0x2B, 0x43, 0x42, 0x3B, 0x16, 0x14,
                                  0x17, 0x1B}, 14);

    cmd_n(0xF0, (const uint8_t[]){0x3C}, 1); // relock extension commands
    cmd_n(0xF0, (const uint8_t[]){0x69}, 1);
    sleep_ms(120);

    // The kit's panel runs in inverted mode: without INVON everything
    // displays as a colour negative (verified on hardware; the vendor
    // demo sends the same command).
    cmd(0x21);
    cmd(0x29);            // display on
    cs(false);

    dma_ch = dma_claim_unused_channel(true);
    dma_cfg = dma_channel_get_default_config(dma_ch);
    channel_config_set_transfer_data_size(&dma_cfg, DMA_SIZE_8);
    channel_config_set_dreq(&dma_cfg, spi_get_dreq(LCD_SPI, true));
    channel_config_set_read_increment(&dma_cfg, true);
    channel_config_set_write_increment(&dma_cfg, false);

    st7796_fill(0x0000);
}

static inline void dma_send(const void *src, uint32_t bytes) {
    dma_channel_configure(dma_ch, &dma_cfg,
                          &spi_get_hw(LCD_SPI)->dr, src, bytes, true);
}

static inline void dma_wait(void) {
    dma_channel_wait_for_finish_blocking(dma_ch);
    while (spi_is_busy(LCD_SPI)) tight_loop_contents();
}

// Expand one 240-px framebuffer row into a 480-px panel line, converting
// native RGB565 to the panel's big-endian byte order.
static void scale_row(const uint16_t *src, uint16_t *dst) {
    for (int x = 0; x < FB_W; x++) {
        uint16_t c = src[x];
        c = (uint16_t)((c >> 8) | (c << 8));
        dst[x * 2] = c;
        dst[x * 2 + 1] = c;
    }
}

void st7796_push_frame(const uint16_t *fb) {
    cs(true);
    set_window(0, 0, LCD_W - 1, LCD_H - 1);

    scale_row(fb, linebuf[0]);
    for (int y = 0; y < FB_H; y++) {
        int cur = y & 1;
        // Each scaled line goes out twice; start the first transfer, then
        // prepare the next row while the second is still on the wire.
        dma_send(linebuf[cur], LCD_W * 2);
        dma_wait();
        dma_send(linebuf[cur], LCD_W * 2);
        if (y + 1 < FB_H)
            scale_row(fb + (y + 1) * FB_W, linebuf[!cur]);
        dma_wait();
    }
    cs(false);
}

void st7796_fill(uint16_t rgb565) {
    uint16_t c = (uint16_t)((rgb565 >> 8) | (rgb565 << 8));
    for (int x = 0; x < LCD_W; x++) linebuf[0][x] = c;

    cs(true);
    set_window(0, 0, LCD_W - 1, LCD_H - 1);
    for (int y = 0; y < LCD_H; y++) {
        dma_send(linebuf[0], LCD_W * 2);
        dma_wait();
    }
    cs(false);
}
