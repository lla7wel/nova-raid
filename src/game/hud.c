#include "game/game.h"
#include "gfx/sprites.h"

#include <stdio.h>

void hud_render(void) {
    gfx_fill_rect(0, 0, FB_W, PLAYFIELD_TOP - 1, C_DBLUE);
    gfx_hline(0, PLAYFIELD_TOP - 1, FB_W, C_DGREY);

    char buf[16];
    snprintf(buf, sizeof buf, "%06lu", (unsigned long)G.score);
    gfx_text(2, 2, buf, C_WHITE, 1);

    if (G.combo > 1) {
        snprintf(buf, sizeof buf, "x%u", G.combo);
        gfx_text(52, 2, buf, C_YELLOW, 1);
    }

    snprintf(buf, sizeof buf, "W%u", G.wave);
    gfx_text(96, 2, buf, C_CYAN, 1);

    for (int i = 0; i < G.pl.lives && i < 5; i++)
        gfx_sprite(126 + i * 9, 2, &spr_heart);

    for (int i = 0; i < G.pl.bombs; i++)
        gfx_sprite(176 + i * 8, 2, &spr_bomb_icon);

    uint32_t best = hiscore_best();
    if (G.score > best) best = G.score;
    snprintf(buf, sizeof buf, "%06lu", (unsigned long)best);
    gfx_text(FB_W - 50, 2, buf, C_GREY, 1);
}

void banner_render(void) {
    if (G.wave_phase != 0) return;
    char buf[20];
    bool boss = wave_is_boss(G.wave);
    // Slide the banner in during the first half of the phase.
    int t = G.wave_t;
    int y = t < 10 ? 60 + (10 - t) * 6 : 60;

    if (boss) {
        if (t & 4) {
            gfx_text_center(y, "! WARNING !", C_RED, 2);
            gfx_text_center(y + 22, "VOID DREADNOUGHT", C_MAGENTA, 1);
        }
    } else {
        snprintf(buf, sizeof buf, "WAVE %u", G.wave);
        gfx_text_center(y, buf, C_CYAN, 2);
        if (G.wave == 1)
            gfx_text_center(y + 22, "GOOD HUNTING", C_GREY, 1);
    }
}
