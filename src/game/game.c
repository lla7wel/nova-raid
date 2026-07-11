#include "game/game.h"
#include "gfx/sprites.h"

#include <stdio.h>
#include <string.h>

// Top-level state machine. game_tick() runs one fixed 40 ms step:
// read input, update the active state, render it, then service the
// audio and LED sequencers.

static const char *menu_items[] = { "START MISSION", "HIGH SCORES", "DIAGNOSTICS" };
#define MENU_N 3

void game_init(void) {
    starfield_init();
    hiscore_load();
    game_enter(ST_SPLASH);
}

void game_enter(state_t s) {
    state_t prev = G.state;
    G.state = s;
    G.frame = 0;

    switch (s) {
        case ST_SPLASH:
            audio_play(SFX_JINGLE_SPLASH);
            ledfx_set(LED_MENU);
            break;
        case ST_MENU:
            if (prev != ST_HISCORES && prev != ST_DIAG) G.menu_sel = 0;
            ledfx_set(LED_MENU);
            hal_led(0, false);
            hal_led(1, false);
            break;
        case ST_PLAY:
            if (prev == ST_MENU) {
                play_reset();
                wave_start(1);
            }
            ledfx_set(wave_is_boss(G.wave) && G.boss.active ? LED_BOSS : LED_PLAY);
            break;
        case ST_PAUSE:
            audio_play(SFX_PAUSE);
            break;
        case ST_GAMEOVER:
            G.score_at_entry = G.score;
            audio_play(SFX_JINGLE_OVER);
            ledfx_set(LED_GAMEOVER);
            hal_led(0, false);
            hal_led(1, false);
            break;
        case ST_ENTRY:
            memcpy(G.entry_name, "AAA", 4);
            G.entry_pos = 0;
            ledfx_set(LED_VICTORY);
            break;
        default:
            break;
    }
}

// --- Splash ---

static void splash_tick(void) {
    starfield_update(140);
    gfx_clear(C_BLACK);
    starfield_render();

    // Ship rises from the bottom during the first second.
    int ship_y = G.frame < 30 ? FB_H - (int)G.frame * 2 : FB_H - 60;
    gfx_sprite(FB_W / 2 - spr_player.w / 2, ship_y, &spr_player);
    const sprite_t *fl = (G.frame & 2) ? &spr_flame0 : &spr_flame1;
    gfx_sprite(FB_W / 2 - fl->w / 2, ship_y + spr_player.h, fl);

    uint16_t pulse = (G.frame / 8) % 2 ? C_CYAN : C_WHITE;
    gfx_text_center(20, "NOVA", pulse, 4);
    gfx_text_center(52, "RAID", C_MAGENTA, 4);
    gfx_text_center(86, "DEEP SPACE DEFENSE", C_GREY, 1);

    if (G.frame > 20 && (G.frame / 12) % 2)
        gfx_text_center(120, "PRESS FIRE", C_YELLOW, 1);
    gfx_text_center(146, "52PI PICO BREADBOARD KIT", C_DGREY, 1);

    if (G.frame > 10 && (G.in.fire_edge || G.in.alt_edge)) {
        audio_play(SFX_MENU_SEL);
        game_enter(ST_MENU);
    }
}

// --- Menu ---

static void menu_tick(void) {
    static int repeat;
    starfield_update(100);

    int dir = G.in.jy > 60 ? -1 : G.in.jy < -60 ? 1 : 0;
    if (dir && repeat == 0) {
        G.menu_sel = (G.menu_sel + dir + MENU_N) % MENU_N;
        audio_play(SFX_MENU_MOVE);
        repeat = 8;
    }
    if (!dir) repeat = 0;
    else if (repeat) repeat--;

    if (G.in.fire_edge) {
        audio_play(SFX_MENU_SEL);
        switch (G.menu_sel) {
            case 0: game_enter(ST_PLAY); return;
            case 1: game_enter(ST_HISCORES); return;
            default: game_enter(ST_DIAG); return;
        }
    }

    gfx_clear(C_BLACK);
    starfield_render();
    gfx_text_center(12, "NOVA", C_CYAN, 3);
    gfx_text_center(36, "RAID", C_MAGENTA, 3);

    for (int i = 0; i < MENU_N; i++) {
        int y = 74 + i * 16;
        bool sel = i == G.menu_sel;
        if (sel) {
            int w = gfx_text_width(menu_items[i], 1);
            gfx_text((FB_W - w) / 2 - 14, y, ">", C_YELLOW, 1);
            gfx_text((FB_W + w) / 2 + 6, y, "<", C_YELLOW, 1);
        }
        gfx_text_center(y, menu_items[i], sel ? C_WHITE : C_GREY, 1);
    }

    gfx_text_center(128, "JOYSTICK MOVE  BTN1 FIRE", C_DGREY, 1);
    gfx_text_center(138, "BTN2 NOVA BOMB  BOTH=PAUSE", C_DGREY, 1);
    char buf[24];
    snprintf(buf, sizeof buf, "BEST %06lu", (unsigned long)hiscore_best());
    gfx_text_center(152, buf, C_DGREY, 1);
    gfx_text(FB_W - 50, 152, "V" NOVA_VERSION, C_DGREY, 1);
}

// --- High-score table ---

static void hiscores_tick(void) {
    starfield_update(100);
    gfx_clear(C_BLACK);
    starfield_render();
    gfx_text_center(10, "HALL OF FAME", C_YELLOW, 2);

    char buf[24];
    for (int i = 0; i < HISCORE_N; i++) {
        snprintf(buf, sizeof buf, "%d  %s  %06lu", i + 1, G.hi.name[i],
                 (unsigned long)G.hi.score[i]);
        uint16_t c = i == 0 ? C_CYAN : C_WHITE;
        gfx_text_center(44 + i * 16, buf, c, 1);
    }
    gfx_text_center(140, "PRESS ANY BUTTON", C_GREY, 1);

    if (G.frame > 10 && (G.in.fire_edge || G.in.alt_edge)) {
        audio_play(SFX_MENU_SEL);
        game_enter(ST_MENU);
    }
}

// --- Play ---

static bool pause_combo(void) {
    return G.in.fire && G.in.alt && (G.in.fire_edge || G.in.alt_edge);
}

static void play_tick(void) {
    if (pause_combo()) {
        game_enter(ST_PAUSE);
        return;
    }

    starfield_update(wave_speed_pct());
    player_update();
    if (G.state != ST_PLAY) return;   // died this frame
    bullets_update();
    if (G.state != ST_PLAY) return;
    enemies_update();
    if (G.state != ST_PLAY) return;
    boss_update();
    if (G.state != ST_PLAY) return;
    pickups_update();
    fx_update();
    wave_update();

    hal_led(0, G.pl.bombs > 0);
    hal_led(1, G.pl.lives == 1 && (G.frame & 8));

    gfx_clear(C_BLACK);
    starfield_render();
    pickups_render();
    enemies_render();
    boss_render();
    bullets_render();
    player_render();
    fx_render();
    hud_render();
    banner_render();
}

// --- Pause ---

static void pause_tick(void) {
    // Re-render the frozen field, dimmed, with a dialog on top.
    gfx_clear(C_BLACK);
    starfield_render();
    pickups_render();
    enemies_render();
    boss_render();
    bullets_render();
    player_render();
    hud_render();
    gfx_dim_rect(0, 0, FB_W, FB_H);

    gfx_fill_rect(50, 56, 140, 52, C_DBLUE);
    gfx_rect(50, 56, 140, 52, C_CYAN);
    gfx_text_center(64, "PAUSED", C_WHITE, 2);
    gfx_text_center(86, "FIRE RESUME", C_GREY, 1);
    gfx_text_center(96, "BOMB QUIT", C_GREY, 1);

    if (G.frame < 5) return;   // let the pause chord release
    if (G.in.fire_edge && !G.in.alt) {
        audio_play(SFX_MENU_SEL);
        game_enter(ST_PLAY);
    } else if (G.in.alt_edge && !G.in.fire) {
        audio_play(SFX_MENU_SEL);
        game_enter(ST_MENU);
    }
}

// --- Game over ---

static void gameover_tick(void) {
    starfield_update(40);
    fx_update();

    gfx_clear(C_BLACK);
    starfield_render();
    fx_render();

    gfx_text_center(34, "GAME OVER", C_RED, 2);
    char buf[24];
    snprintf(buf, sizeof buf, "SCORE %06lu", (unsigned long)G.score);
    gfx_text_center(64, buf, C_WHITE, 1);
    snprintf(buf, sizeof buf, "WAVE %u", G.wave);
    gfx_text_center(76, buf, C_CYAN, 1);

    if (hiscore_qualifies(G.score))
        gfx_text_center(96, "NEW HIGH SCORE!", (G.frame & 8) ? C_YELLOW : C_ORANGE, 1);

    if (G.frame > 25 && (G.frame / 12) % 2)
        gfx_text_center(124, "PRESS FIRE", C_GREY, 1);

    if (G.frame > 25 && G.in.fire_edge)
        game_enter(hiscore_qualifies(G.score) ? ST_ENTRY : ST_MENU);
}

// --- High-score initials entry ---

static void entry_tick(void) {
    static int repeat;
    starfield_update(60);

    int dir = G.in.jy > 60 ? 1 : G.in.jy < -60 ? -1 : 0;
    if (dir && repeat == 0) {
        char *c = &G.entry_name[G.entry_pos];
        // Cycle A-Z, 0-9, space.
        static const char set[] = "ABCDEFGHIJKLMNOPQRSTUVWXYZ0123456789 ";
        const char *p = strchr(set, *c);
        int idx = p ? (int)(p - set) : 0;
        int n = (int)sizeof(set) - 1;
        *c = set[(idx + dir + n) % n];
        audio_play(SFX_MENU_MOVE);
        repeat = 6;
    }
    if (!dir) repeat = 0;
    else if (repeat) repeat--;

    if (G.in.fire_edge) {
        audio_play(SFX_MENU_SEL);
        if (++G.entry_pos >= 3) {
            hiscore_insert(G.entry_name, G.score_at_entry);
            game_enter(ST_HISCORES);
            return;
        }
    }
    if (G.in.alt_edge && G.entry_pos > 0) {
        G.entry_pos--;
        audio_play(SFX_MENU_MOVE);
    }

    gfx_clear(C_BLACK);
    starfield_render();
    gfx_text_center(20, "NEW HIGH SCORE", C_YELLOW, 2);
    char buf[24];
    snprintf(buf, sizeof buf, "%06lu", (unsigned long)G.score_at_entry);
    gfx_text_center(46, buf, C_WHITE, 2);
    gfx_text_center(74, "ENTER YOUR CALLSIGN", C_GREY, 1);

    for (int i = 0; i < 3; i++) {
        int x = FB_W / 2 - 30 + i * 20;
        bool cur = i == G.entry_pos;
        char s[2] = { G.entry_name[i], 0 };
        gfx_text(x, 96, s, cur ? C_CYAN : C_WHITE, 2);
        if (cur && (G.frame & 8))
            gfx_hline(x, 114, 16, C_CYAN);
    }
    gfx_text_center(134, "STICK: LETTER  FIRE: NEXT", C_DGREY, 1);
}

// --- Dispatch ---

void game_tick(void) {
    hal_input_read(&G.in);
    G.frame++;

    switch (G.state) {
        case ST_SPLASH:   splash_tick();   break;
        case ST_MENU:     menu_tick();     break;
        case ST_HISCORES: hiscores_tick(); break;
        case ST_DIAG:     diag_update();   diag_render(); break;
        case ST_PLAY:     play_tick();     break;
        case ST_PAUSE:    pause_tick();    break;
        case ST_GAMEOVER: gameover_tick(); break;
        case ST_ENTRY:    entry_tick();    break;
    }

    audio_update();
    if (G.state != ST_DIAG) ledfx_update();
}
