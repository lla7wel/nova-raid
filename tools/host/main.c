// Frame-capture harness: runs the real game core on the host and writes
// selected frames as PPM images (2x upscaled, like the panel shows them).
// Usage: capture <outdir>

#include "game/game.h"
#include "gfx/gfx.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

extern input_t host_input;

static uint16_t fb[FB_W * FB_H];

static void save_ppm(const char *dir, const char *name) {
    char path[512];
    snprintf(path, sizeof path, "%s/%s.ppm", dir, name);
    FILE *f = fopen(path, "wb");
    if (!f) { perror(path); exit(1); }
    fprintf(f, "P6\n%d %d\n255\n", FB_W * 2, FB_H * 2);
    for (int y = 0; y < FB_H * 2; y++) {
        for (int x = 0; x < FB_W * 2; x++) {
            uint16_t c = fb[(y / 2) * FB_W + (x / 2)];
            uint8_t px[3] = {
                (uint8_t)(((c >> 11) & 0x1F) << 3),
                (uint8_t)(((c >> 5) & 0x3F) << 2),
                (uint8_t)((c & 0x1F) << 3),
            };
            fwrite(px, 1, 3, f);
        }
    }
    fclose(f);
    printf("wrote %s\n", path);
}

static void ticks(int n) {
    for (int i = 0; i < n; i++) game_tick();
}

static void neutral(void) {
    memset(&host_input, 0, sizeof host_input);
}

static void tap_fire(void) {
    host_input.fire = true;
    ticks(1);
    host_input.fire = false;
    ticks(1);
}

int main(int argc, char **argv) {
    const char *dir = argc > 1 ? argv[1] : ".";

    gfx_set_target(fb);
    game_init();
    neutral();

    // Splash with the ship settled and PRESS FIRE visible.
    ticks(45);
    save_ppm(dir, "01-splash");

    // Menu.
    tap_fire();
    ticks(30);
    save_ppm(dir, "02-menu");

    // Early gameplay: hold fire, weave the ship while wave 1 arrives.
    tap_fire();               // START MISSION
    ticks(55);                // wave banner plays out
    host_input.fire = true;
    for (int i = 0; i < 130; i++) {
        host_input.jx = (int8_t)(i % 80 < 40 ? 90 : -90);
        host_input.jy = (int8_t)(i % 50 < 25 ? 35 : -35);
        ticks(1);
    }
    save_ppm(dir, "03-gameplay");
    neutral();

    // Boss encounter: jump the wave counter, let the warning play.
    wave_start(5);
    ticks(30);
    save_ppm(dir, "04-boss-warning");
    ticks(75);                // dreadnought descends, opens fire
    host_input.fire = true;
    for (int i = 0; i < 90; i++) {
        host_input.jx = (int8_t)(i % 60 < 30 ? 70 : -70);
        ticks(1);
    }
    save_ppm(dir, "05-boss-fight");
    neutral();

    // Pause dialog.
    host_input.fire = host_input.alt = true;
    ticks(1);
    neutral();
    ticks(8);
    save_ppm(dir, "06-pause");

    // Game over via forced player death.
    host_input.fire = true;   // resume
    ticks(2);
    neutral();
    G.pl.inv = 0;
    G.pl.shield = false;
    while (G.state == ST_PLAY) {
        G.pl.inv = 0;
        player_hit();
    }
    G.score = 12480;
    ticks(40);
    save_ppm(dir, "07-gameover");

    // Initials entry.
    host_input.fire = true;
    ticks(1);
    neutral();
    if (G.state == ST_ENTRY) {
        ticks(10);
        save_ppm(dir, "08-entry");
        for (int i = 0; i < 3; i++) tap_fire();
    }

    // Hall of fame with the fresh score inserted.
    ticks(20);
    save_ppm(dir, "09-hiscores");

    // Diagnostics.
    game_enter(ST_MENU);
    ticks(5);
    game_enter(ST_DIAG);
    host_input.jx = 70;
    host_input.jy = 40;
    host_input.fire = true;
    ticks(12);
    save_ppm(dir, "10-diagnostics");

    return 0;
}
