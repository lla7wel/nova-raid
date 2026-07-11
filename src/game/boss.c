#include "game/game.h"
#include "gfx/sprites.h"

// The Void Dreadnought. Phases by remaining HP:
//   >66%  sweeps horizontally firing 3-shot spreads
//   >33%  holds centre, launches drone escorts and aimed shots
//   <=33% desperation radial barrages while lurching side to side

#define BOSS_Y 34

void boss_start(void) {
    boss_t *bs = &G.boss;
    int bossnum = G.wave / 5;
    bs->active = true;
    bs->phase = 0;
    bs->hp_max = (int16_t)(60 + (bossnum - 1) * 30);
    bs->hp = bs->hp_max;
    bs->b = (body_t){ FP(FB_W / 2), FP(-20), FP(0.8), FP(0.6) };
    bs->t = 0;
    bs->flash = 0;
}

void boss_damage(int dmg) {
    boss_t *bs = &G.boss;
    if (!bs->active || bs->t < 60) return;
    bs->hp -= dmg;
    bs->flash = 3;
    if (bs->hp > 0) return;

    bs->active = false;
    int x = FP_INT(bs->b.x), y = FP_INT(bs->b.y);
    for (int i = 0; i < 5; i++)
        fx_explosion(x - 14 + (int)(hal_rand() % 28),
                     y - 8 + (int)(hal_rand() % 16));
    fx_sparks(x, y, C_ORANGE, 10);
    fx_sparks(x, y, C_YELLOW, 8);
    audio_play(SFX_BIG_EXPLODE);
    ledfx_event(LED_BOMB);
    add_score(1500 + (uint32_t)(G.wave / 5) * 500);
    if (G.pl.bombs < 4) G.pl.bombs++;
}

static void radial_burst(int cx, int cy, int n, int32_t speed) {
    for (int i = 0; i < n; i++) {
        for (int j = 0; j < MAX_EBUL; j++) {
            if (G.ebul[j].alive) continue;
            // Directions from the 64-step unit circle used by enemies.
            extern const int16_t boss_sin64[64];
            int a = i * 64 / n;
            int32_t vx = boss_sin64[a & 63] * speed / 256;
            int32_t vy = boss_sin64[(a + 16) & 63] * speed / 256;
            if (vy < FP(0.2)) vy += FP(0.5); // bias downward, keep it fair
            G.ebul[j].alive = true;
            G.ebul[j].b = (body_t){ FP(cx), FP(cy), vx, vy };
            break;
        }
    }
}

const int16_t boss_sin64[64] = {
      0,  25,  50,  74,  98, 121, 142, 162, 181, 198, 213, 226, 237, 245,
    251, 255, 256, 255, 251, 245, 237, 226, 213, 198, 181, 162, 142, 121,
     98,  74,  50,  25,   0, -25, -50, -74, -98,-121,-142,-162,-181,-198,
   -213,-226,-237,-245,-251,-255,-256,-255,-251,-245,-237,-226,-213,-198,
   -181,-162,-142,-121, -98, -74, -50, -25
};

void boss_update(void) {
    boss_t *bs = &G.boss;
    if (!bs->active) return;
    bs->t++;
    if (bs->flash) bs->flash--;

    int x = FP_INT(bs->b.x), y = FP_INT(bs->b.y);
    uint8_t phase = bs->hp > bs->hp_max * 2 / 3 ? 0
                  : bs->hp > bs->hp_max / 3     ? 1 : 2;
    if (phase != bs->phase) {
        bs->phase = phase;
        audio_play(SFX_BOSS_WARN);
        fx_sparks(x, y, C_MAGENTA, 8);
    }

    // Entrance descent.
    if (y < BOSS_Y) {
        bs->b.y += bs->b.vy;
    } else {
        switch (phase) {
            case 0:
                bs->b.x += bs->b.vx;
                if (x < 26 || x > FB_W - 26) bs->b.vx = -bs->b.vx;
                if (bs->t % 36 == 0) {
                    enemy_fire(x - 10, y + 10, FP(1.6));
                    enemy_fire(x, y + 12, FP(1.8));
                    enemy_fire(x + 10, y + 10, FP(1.6));
                }
                break;
            case 1:
                // Drift home to centre, deploy escorts.
                bs->b.x += (FP(FB_W / 2) - bs->b.x) / 32;
                if (bs->t % 60 == 0) {
                    spawn_enemy(EN_DRONE, x - 24, y + 4);
                    spawn_enemy(EN_DRONE, x + 24, y + 4);
                }
                if (bs->t % 28 == 0)
                    enemy_fire(x, y + 12, FP(2.0));
                break;
            default:
                bs->b.x += bs->b.vx * 2;
                if (x < 30 || x > FB_W - 30) bs->b.vx = -bs->b.vx;
                if (bs->t % 44 == 0)
                    radial_burst(x, y + 6, 10, FP(1.4));
                if (bs->t % 44 == 22)
                    enemy_fire(x, y + 12, FP(2.2));
                break;
        }
    }

    // Player bullets vs boss hull.
    int hw = spr_boss.w / 2 - 2, hh = spr_boss.h / 2 - 2;
    for (int j = 0; j < MAX_PBUL; j++) {
        pbullet_t *b = &G.pbul[j];
        if (!b->alive) continue;
        int bx = FP_INT(b->b.x), by = FP_INT(b->b.y);
        if (bx > x - hw && bx < x + hw && by > y - hh && by < y + hh) {
            b->alive = false;
            fx_sparks(bx, by, C_WHITE, 2);
            audio_play(SFX_HIT);
            boss_damage(1);
        }
    }

    // Ramming the hull hurts.
    int px = FP_INT(G.pl.x), py = FP_INT(G.pl.y);
    if (px > x - hw - 4 && px < x + hw + 4 && py > y - hh - 4 && py < y + hh + 4)
        player_hit();
}

void boss_render(void) {
    boss_t *bs = &G.boss;
    if (!bs->active) return;
    int x = FP_INT(bs->b.x) - spr_boss.w / 2;
    int y = FP_INT(bs->b.y) - spr_boss.h / 2;
    if (bs->flash)
        gfx_sprite_tint(x, y, &spr_boss, C_WHITE);
    else
        gfx_sprite(x, y, &spr_boss);

    // Health bar under the HUD.
    int bar_w = FB_W - 40;
    int fill = bs->hp > 0 ? bs->hp * bar_w / bs->hp_max : 0;
    gfx_rect(19, PLAYFIELD_TOP + 2, bar_w + 2, 5, C_DGREY);
    gfx_fill_rect(20, PLAYFIELD_TOP + 3, fill, 3,
                  bs->phase == 2 ? C_RED : C_MAGENTA);
}
