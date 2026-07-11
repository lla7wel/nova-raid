#include "game/game.h"

// Wave sequencing. Non-boss waves spawn a budget of enemies through one of
// several entry patterns; every 5th wave is the Void Dreadnought.

#define BANNER_FRAMES  (2 * GAME_FPS)

bool wave_is_boss(uint8_t wave) {
    return wave % 5 == 0;
}

int wave_speed_pct(void) {
    int pct = 100 + (G.wave - 1) * 6;
    return pct > 220 ? 220 : pct;
}

void wave_start(uint8_t wave) {
    G.wave = wave;
    G.wave_phase = 0;
    G.wave_t = 0;
    G.spawn_left = wave_is_boss(wave) ? 0 : (uint8_t)(6 + wave * 2);
    if (G.spawn_left > 26) G.spawn_left = 26;
    G.pattern = hal_rand() % 4;
    audio_play(wave_is_boss(wave) ? SFX_BOSS_WARN : SFX_WAVE);
    if (wave_is_boss(wave)) ledfx_set(LED_BOSS);
}

// Pick an enemy type with wave-dependent weights: later waves shift from
// drones toward darters, sentries and asteroids.
static etype_t pick_type(void) {
    uint32_t r = hal_rand() % 100;
    int w = G.wave;
    if (r < (uint32_t)(50 > 20 + w * 3 ? 50 - w * 3 : 20)) return EN_DRONE;
    if (r < 65) return EN_DARTER;
    if (r < 85) return w >= 2 ? EN_SENTRY : EN_DRONE;
    return w >= 3 ? EN_ROID_L : EN_DARTER;
}

static void spawn_next(void) {
    etype_t t = pick_type();
    int x;
    switch (G.pattern) {
        case 0:  // random across the top
            x = 14 + hal_rand() % (FB_W - 28);
            break;
        case 1:  // marching columns
            x = 24 + (G.spawn_left % 5) * (FB_W - 48) / 4;
            break;
        case 2:  // V formation
            x = FB_W / 2 + ((G.spawn_left & 1) ? 1 : -1) * (G.spawn_left * 9);
            break;
        default: // alternating flanks
            x = (G.spawn_left & 1) ? 18 : FB_W - 18;
            break;
    }
    if (x < 10) x = 10;
    if (x > FB_W - 10) x = FB_W - 10;
    spawn_enemy(t, x, PLAYFIELD_TOP - 6);
}

static bool field_clear(void) {
    for (int i = 0; i < MAX_ENEM; i++)
        if (G.enem[i].alive) return false;
    return true;
}

void wave_update(void) {
    G.wave_t++;

    switch (G.wave_phase) {
        case 0:  // banner
            if (G.wave_t >= BANNER_FRAMES) {
                G.wave_phase = 1;
                G.wave_t = 0;
                if (wave_is_boss(G.wave)) boss_start();
            }
            break;

        case 1:  // spawning
            if (wave_is_boss(G.wave)) {
                if (!G.boss.active) G.wave_phase = 2;
                break;
            }
            {
                // Spawn cadence tightens as waves progress.
                uint16_t gap = 30 > 10 + G.wave ? 30 - G.wave : 10;
                if (G.wave_t % gap == 0 && G.spawn_left) {
                    spawn_next();
                    G.spawn_left--;
                }
                if (!G.spawn_left) G.wave_phase = 2;
            }
            break;

        case 2:  // waiting for the field to clear
            if (wave_is_boss(G.wave)) {
                if (!G.boss.active && field_clear()) {
                    G.wave_phase = 3;
                    G.wave_t = 0;
                    add_score(500 + G.wave * 50);
                    audio_play(SFX_JINGLE_VICTORY);
                    ledfx_set(LED_PLAY);
                    ledfx_event(LED_POWERUP);
                }
            } else if (field_clear()) {
                G.wave_phase = 3;
                G.wave_t = 0;
                add_score(100 + G.wave * 20);
            }
            break;

        default: // brief breather, then next wave
            if (G.wave_t >= GAME_FPS)
                wave_start(G.wave + 1);
            break;
    }
}
