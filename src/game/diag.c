#include "game/game.h"

#include <stdio.h>

// Hardware diagnostics screen, reachable from the main menu. Exercises
// every peripheral so wiring, orientation and config.h flags can be
// verified without any other equipment.

void diag_update(void) {
    // RGB LED cycles red / green / blue / white, one second each.
    static const uint8_t cols[4][3] = {
        {255, 0, 0}, {0, 255, 0}, {0, 0, 255}, {255, 255, 255}
    };
    const uint8_t *c = cols[(G.frame / GAME_FPS) % 4];
    hal_rgb(c[0], c[1], c[2]);

    // Panel LEDs alternate.
    hal_led(0, (G.frame / GAME_FPS) & 1);
    hal_led(1, !((G.frame / GAME_FPS) & 1));

    // Hold FIRE for a tone sweep to verify the buzzer.
    if (G.in.fire)
        hal_tone((uint16_t)(400 + (G.frame % 50) * 40), 150);
    else if (G.in.alt)
        hal_tone(880, 150);
    else
        hal_tone(0, 0);

    // Hold both buttons ~1 s to exit.
    static int exit_hold;
    if (G.in.fire && G.in.alt) {
        if (++exit_hold > GAME_FPS) {
            exit_hold = 0;
            hal_tone(0, 0);
            hal_led(0, false);
            hal_led(1, false);
            game_enter(ST_MENU);
        }
    } else {
        exit_hold = 0;
    }
}

void diag_render(void) {
    char buf[40];
    gfx_clear(C_BLACK);
    gfx_text_center(3, "DIAGNOSTICS", C_YELLOW, 1);

    // Raw and scaled joystick readout.
    uint16_t rx, ry;
    hal_input_raw(&rx, &ry);
    snprintf(buf, sizeof buf, "ADC X %4u  Y %4u", rx, ry);
    gfx_text(8, 18, buf, C_WHITE, 1);
    snprintf(buf, sizeof buf, "JOY %4d %4d", G.in.jx, G.in.jy);
    gfx_text(8, 28, buf, C_CYAN, 1);

    // Crosshair box: the dot must follow the stick; up must be up.
    int bx = 170, by = 18, bs = 52;
    gfx_rect(bx, by, bs, bs, C_DGREY);
    gfx_hline(bx, by + bs / 2, bs, C_DGREY);
    gfx_vline(bx + bs / 2, by, bs, C_DGREY);
    int dx = bx + bs / 2 + G.in.jx * (bs / 2 - 3) / 127;
    int dy = by + bs / 2 - G.in.jy * (bs / 2 - 3) / 127;
    gfx_fill_rect(dx - 2, dy - 2, 5, 5, C_GREEN);
    gfx_text(bx + 4, by + 4, "UP", C_GREY, 1);

    snprintf(buf, sizeof buf, "BTN1 FIRE [%c]", G.in.fire ? 'X' : ' ');
    gfx_text(8, 42, buf, G.in.fire ? C_GREEN : C_GREY, 1);
    snprintf(buf, sizeof buf, "BTN2 ALT  [%c]", G.in.alt ? 'X' : ' ');
    gfx_text(8, 52, buf, G.in.alt ? C_GREEN : C_GREY, 1);

    gfx_text(8, 68, "RGB+LED CYCLE AUTO", C_WHITE, 1);
    gfx_text(8, 78, "HOLD FIRE: TONE SWEEP", C_WHITE, 1);

    // Colour bars and orientation arrow for the display test. The arrow
    // must point toward the top edge; if not, adjust LCD_ROTATION.
    static const uint16_t bars[8] = {
        C_WHITE, C_YELLOW, C_CYAN, C_GREEN, C_MAGENTA, C_RED, C_BLUE, C_BLACK
    };
    for (int i = 0; i < 8; i++)
        gfx_fill_rect(8 + i * 28, 96, 28, 22, bars[i]);
    gfx_rect(8, 96, 224, 22, C_DGREY);

    gfx_text_center(126, "ARROW MUST POINT UP", C_GREY, 1);
    int ax = FB_W / 2, ay = 138;
    gfx_vline(ax, ay, 12, C_YELLOW);
    gfx_pixel(ax - 1, ay + 1, C_YELLOW);
    gfx_pixel(ax + 1, ay + 1, C_YELLOW);
    gfx_pixel(ax - 2, ay + 2, C_YELLOW);
    gfx_pixel(ax + 2, ay + 2, C_YELLOW);

    gfx_text_center(152, "HOLD BOTH BUTTONS TO EXIT", C_ORANGE, 1);
}
