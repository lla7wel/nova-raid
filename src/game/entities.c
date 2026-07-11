#include "game/game.h"
#include "gfx/sprites.h"

// All entity positions are sprite centres in 24.8 fixed point.

#define PLAYER_SPEED    FP(2.3)
#define PBUL_SPEED      FP(5.0)
#define FIRE_CD         7
#define FIRE_CD_RAPID   3
#define INV_FRAMES      45
#define COMBO_WINDOW    40   // frames to keep the multiplier alive
#define POWERUP_FRAMES  (8 * GAME_FPS)

game_t G;

static const sprite_t *enemy_sprite(etype_t t) {
    switch (t) {
        case EN_DRONE:  return &spr_drone;
        case EN_DARTER: return &spr_darter;
        case EN_SENTRY: return &spr_sentry;
        case EN_ROID_L: return &spr_roid_l;
        default:        return &spr_roid_s;
    }
}

static int enemy_score(etype_t t) {
    switch (t) {
        case EN_DRONE:  return 50;
        case EN_DARTER: return 80;
        case EN_SENTRY: return 120;
        case EN_ROID_L: return 30;
        default:        return 15;
    }
}

void play_reset(void) {
    player_t *pl = &G.pl;
    pl->x = FP(FB_W / 2);
    pl->y = FP(FB_H - 20);
    pl->lives = 3;
    pl->bombs = 2;
    pl->shield = false;
    pl->inv = 30;
    pl->fire_cd = 0;
    pl->rapid_t = pl->spread_t = 0;

    for (int i = 0; i < MAX_PBUL; i++) G.pbul[i].alive = false;
    for (int i = 0; i < MAX_EBUL; i++) G.ebul[i].alive = false;
    for (int i = 0; i < MAX_ENEM; i++) G.enem[i].alive = false;
    for (int i = 0; i < MAX_PUPS; i++) G.pups[i].alive = false;
    for (int i = 0; i < MAX_FX; i++) G.fx[i].alive = false;
    G.boss.active = false;

    G.score = 0;
    G.combo = 1;
    G.combo_t = 0;
    G.wave = 0;
}

void add_score(uint32_t base) {
    G.score += base * G.combo;
    if (G.combo < 5) {
        if (G.combo_t > 0) G.combo++;
    }
    G.combo_t = COMBO_WINDOW;
}

// --- Effects ---

static fx_t *fx_alloc(void) {
    for (int i = 0; i < MAX_FX; i++)
        if (!G.fx[i].alive) return &G.fx[i];
    return &G.fx[0];
}

void fx_explosion(int x, int y) {
    fx_t *f = fx_alloc();
    f->alive = true;
    f->b.x = FP(x);
    f->b.y = FP(y);
    f->b.vx = f->b.vy = 0;
    f->age = 0;
    f->life = 9;
    f->color = 0;
}

void fx_sparks(int x, int y, uint16_t color, int n) {
    for (int i = 0; i < n; i++) {
        fx_t *f = fx_alloc();
        f->alive = true;
        f->b.x = FP(x);
        f->b.y = FP(y);
        f->b.vx = (int32_t)(hal_rand() % FP(3)) - FP(1.5);
        f->b.vy = (int32_t)(hal_rand() % FP(3)) - FP(1.5);
        f->age = 0;
        f->life = (uint8_t)(6 + hal_rand() % 8);
        f->color = color;
    }
}

void fx_update(void) {
    for (int i = 0; i < MAX_FX; i++) {
        fx_t *f = &G.fx[i];
        if (!f->alive) continue;
        f->b.x += f->b.vx;
        f->b.y += f->b.vy;
        if (++f->age >= f->life) f->alive = false;
    }
}

void fx_render(void) {
    for (int i = 0; i < MAX_FX; i++) {
        fx_t *f = &G.fx[i];
        if (!f->alive) continue;
        int x = FP_INT(f->b.x), y = FP_INT(f->b.y);
        if (f->color) {
            gfx_pixel(x, y, f->color);
        } else {
            const sprite_t *s = f->age < 3 ? &spr_expl0
                              : f->age < 6 ? &spr_expl1 : &spr_expl2;
            gfx_sprite(x - s->w / 2, y - s->h / 2, s);
        }
    }
}

// --- Player ---

static void fire_bullet(int32_t x, int32_t y, int32_t vx) {
    for (int i = 0; i < MAX_PBUL; i++) {
        if (G.pbul[i].alive) continue;
        G.pbul[i].alive = true;
        G.pbul[i].b = (body_t){ x, y, vx, -PBUL_SPEED };
        return;
    }
}

void player_update(void) {
    player_t *pl = &G.pl;

    pl->x += (int32_t)G.in.jx * PLAYER_SPEED / 127;
    pl->y -= (int32_t)G.in.jy * PLAYER_SPEED / 127;
    if (pl->x < FP(8)) pl->x = FP(8);
    if (pl->x > FP(FB_W - 8)) pl->x = FP(FB_W - 8);
    if (pl->y < FP(PLAYFIELD_TOP + 8)) pl->y = FP(PLAYFIELD_TOP + 8);
    if (pl->y > FP(FB_H - 7)) pl->y = FP(FB_H - 7);

    if (pl->inv) pl->inv--;
    if (pl->fire_cd) pl->fire_cd--;
    if (pl->rapid_t) pl->rapid_t--;
    if (pl->spread_t) pl->spread_t--;
    if (G.combo_t) {
        if (--G.combo_t == 0) G.combo = 1;
    }

    if (G.in.fire && !pl->fire_cd) {
        pl->fire_cd = pl->rapid_t ? FIRE_CD_RAPID : FIRE_CD;
        fire_bullet(pl->x, pl->y - FP(6), 0);
        if (pl->spread_t) {
            fire_bullet(pl->x - FP(4), pl->y - FP(4), -FP(1.1));
            fire_bullet(pl->x + FP(4), pl->y - FP(4), FP(1.1));
        }
        audio_play(SFX_LASER);
    }

    if (G.in.alt_edge) use_bomb();
}

void use_bomb(void) {
    if (!G.pl.bombs) return;
    G.pl.bombs--;
    audio_play(SFX_BOMB);
    ledfx_event(LED_BOMB);
    for (int i = 0; i < MAX_EBUL; i++) G.ebul[i].alive = false;
    for (int i = 0; i < MAX_ENEM; i++) {
        enemy_t *e = &G.enem[i];
        if (!e->alive) continue;
        e->hp -= 4;
        e->flash = 4;
        if (e->hp <= 0) enemy_kill(e, true);
    }
    if (G.boss.active) boss_damage(10);
}

void player_render(void) {
    player_t *pl = &G.pl;
    if (pl->inv & 2) return;   // respawn blink

    int x = FP_INT(pl->x) - spr_player.w / 2;
    int y = FP_INT(pl->y) - spr_player.h / 2;
    gfx_sprite(x, y, &spr_player);

    const sprite_t *fl = (G.frame & 2) ? &spr_flame0 : &spr_flame1;
    gfx_sprite(FP_INT(pl->x) - fl->w / 2, y + spr_player.h, fl);

    if (pl->shield) {
        int cx = FP_INT(pl->x), cy = FP_INT(pl->y);
        uint16_t c = (G.frame & 4) ? C_CYAN : C_BLUE;
        gfx_rect(cx - 9, cy - 8, 19, 17, c);
    }
}

void player_hit(void) {
    player_t *pl = &G.pl;
    if (pl->inv) return;
    if (pl->shield) {
        pl->shield = false;
        pl->inv = 20;
        audio_play(SFX_HIT);
        ledfx_event(LED_SHIELD);
        fx_sparks(FP_INT(pl->x), FP_INT(pl->y), C_CYAN, 6);
        return;
    }
    pl->lives--;
    pl->inv = INV_FRAMES;
    pl->rapid_t = pl->spread_t = 0;
    audio_play(SFX_PLAYER_HIT);
    ledfx_event(LED_DAMAGE);
    fx_explosion(FP_INT(pl->x), FP_INT(pl->y));
    fx_sparks(FP_INT(pl->x), FP_INT(pl->y), C_ORANGE, 8);
    if (pl->lives == 0) game_enter(ST_GAMEOVER);
}

// --- Bullets ---

void enemy_fire(int x, int y, int32_t speed_fp) {
    for (int i = 0; i < MAX_EBUL; i++) {
        if (G.ebul[i].alive) continue;
        // Aim at the player with slight spread.
        int32_t dx = G.pl.x - FP(x);
        int32_t dy = G.pl.y - FP(y);
        int adx = dx < 0 ? -dx : dx, ady = dy < 0 ? -dy : dy;
        int32_t mag = adx > ady ? adx + ady / 2 : ady + adx / 2; // ~|v|
        if (mag < FP(1)) mag = FP(1);
        int32_t vx = (int64_t)dx * speed_fp / mag;
        int32_t vy = (int64_t)dy * speed_fp / mag;
        vx += (int32_t)(hal_rand() % FP(0.5)) - FP(0.25);
        G.ebul[i].alive = true;
        G.ebul[i].b = (body_t){ FP(x), FP(y), vx, vy };
        return;
    }
}

static bool hit_player(int x, int y, int hw, int hh) {
    int px = FP_INT(G.pl.x), py = FP_INT(G.pl.y);
    return x + hw > px - 5 && x - hw < px + 5 &&
           y + hh > py - 4 && y - hh < py + 4;
}

void bullets_update(void) {
    for (int i = 0; i < MAX_PBUL; i++) {
        pbullet_t *b = &G.pbul[i];
        if (!b->alive) continue;
        b->b.x += b->b.vx;
        b->b.y += b->b.vy;
        if (b->b.y < FP(PLAYFIELD_TOP - 4) || b->b.x < 0 || b->b.x > FP(FB_W))
            b->alive = false;
    }
    for (int i = 0; i < MAX_EBUL; i++) {
        ebullet_t *b = &G.ebul[i];
        if (!b->alive) continue;
        b->b.x += b->b.vx;
        b->b.y += b->b.vy;
        int x = FP_INT(b->b.x), y = FP_INT(b->b.y);
        if (y > FB_H + 4 || y < PLAYFIELD_TOP - 4 || x < -4 || x > FB_W + 4) {
            b->alive = false;
            continue;
        }
        if (hit_player(x, y, 2, 2)) {
            b->alive = false;
            player_hit();
        }
    }
}

void bullets_render(void) {
    for (int i = 0; i < MAX_PBUL; i++) {
        if (!G.pbul[i].alive) continue;
        int x = FP_INT(G.pbul[i].b.x), y = FP_INT(G.pbul[i].b.y);
        gfx_fill_rect(x - 1, y - 3, 2, 6, C_CYAN);
        gfx_pixel(x - 1, y + 3, C_WHITE);
        gfx_pixel(x, y + 3, C_WHITE);
    }
    for (int i = 0; i < MAX_EBUL; i++) {
        if (!G.ebul[i].alive) continue;
        int x = FP_INT(G.ebul[i].b.x), y = FP_INT(G.ebul[i].b.y);
        gfx_fill_rect(x - 1, y - 1, 3, 3, (G.frame & 2) ? C_YELLOW : C_ORANGE);
    }
}

// --- Enemies ---

bool spawn_enemy(etype_t type, int x, int y) {
    for (int i = 0; i < MAX_ENEM; i++) {
        enemy_t *e = &G.enem[i];
        if (e->alive) continue;
        e->alive = true;
        e->type = type;
        e->b = (body_t){ FP(x), FP(y), 0, 0 };
        e->t = 0;
        e->p0 = x;
        e->p1 = 0;
        e->flash = 0;
        switch (type) {
            case EN_DRONE:  e->hp = 1; e->b.vy = FP(0.7); break;
            case EN_DARTER: e->hp = 1; e->b.vy = FP(0.5); break;
            case EN_SENTRY: e->hp = 2; e->b.vy = FP(0.35);
                            e->b.vx = (x < FB_W / 2) ? FP(0.6) : -FP(0.6);
                            break;
            case EN_ROID_L: e->hp = 3 + G.wave / 6; e->b.vy = FP(0.45);
                            e->b.vx = (int32_t)(hal_rand() % FP(0.8)) - FP(0.4);
                            break;
            case EN_ROID_S: e->hp = 1; e->b.vy = FP(0.8);
                            e->b.vx = (int32_t)(hal_rand() % FP(1.6)) - FP(0.8);
                            break;
            default: break;
        }
        int pct = wave_speed_pct();
        e->b.vx = e->b.vx * pct / 100;
        e->b.vy = e->b.vy * pct / 100;
        return true;
    }
    return false;
}

static void drop_powerup(int x, int y) {
    if (hal_rand() % 100 >= 14) return;
    for (int i = 0; i < MAX_PUPS; i++) {
        pickup_t *p = &G.pups[i];
        if (p->alive) continue;
        uint32_t r = hal_rand() % 100;
        p->kind = r < 28 ? PU_SPREAD
                : r < 56 ? PU_RAPID
                : r < 76 ? PU_SHIELD
                : r < 94 ? PU_BOMB : PU_LIFE;
        p->alive = true;
        p->b = (body_t){ FP(x), FP(y), 0, FP(0.6) };
        return;
    }
}

void enemy_kill(enemy_t *e, bool scored) {
    e->alive = false;
    int x = FP_INT(e->b.x), y = FP_INT(e->b.y);
    fx_explosion(x, y);
    fx_sparks(x, y, C_ORANGE, 4);
    audio_play(SFX_EXPLODE);
    if (scored) {
        add_score(enemy_score(e->type));
        if (e->type == EN_ROID_L) {
            spawn_enemy(EN_ROID_S, x - 5, y);
            spawn_enemy(EN_ROID_S, x + 5, y);
        }
        drop_powerup(x, y);
    }
}

// Sine lookup, amplitude 256, one period = 64 steps.
static const int16_t sin64[64] = {
      0,  25,  50,  74,  98, 121, 142, 162, 181, 198, 213, 226, 237, 245,
    251, 255, 256, 255, 251, 245, 237, 226, 213, 198, 181, 162, 142, 121,
     98,  74,  50,  25,   0, -25, -50, -74, -98,-121,-142,-162,-181,-198,
   -213,-226,-237,-245,-251,-255,-256,-255,-251,-245,-237,-226,-213,-198,
   -181,-162,-142,-121, -98, -74, -50, -25
};

static void enemy_behave(enemy_t *e) {
    int pct = wave_speed_pct();
    switch (e->type) {
        case EN_DRONE:
            e->b.x = FP(e->p0) + sin64[(e->t / 2) & 63] * 18;
            break;
        case EN_DARTER:
            if (e->p1 == 0 && FP_INT(e->b.y) > 40 + (int)(hal_rand() % 20)) {
                // Lock on and dive at the player's current position.
                e->p1 = 1;
                int32_t dx = G.pl.x - e->b.x;
                e->b.vx = dx / 40;
                e->b.vy = FP(2.4) * pct / 100;
                audio_play(SFX_HIT);
            }
            break;
        case EN_SENTRY: {
            int x = FP_INT(e->b.x);
            if (x < 10 || x > FB_W - 10) e->b.vx = -e->b.vx;
            uint16_t interval = 50 > 20 + G.wave * 2 ? 50 - G.wave * 2 : 20;
            if (e->t % interval == 0 && e->t > 20)
                enemy_fire(x, FP_INT(e->b.y) + 5, FP(1.5) * pct / 100);
            break;
        }
        default:
            break;
    }
}

void enemies_update(void) {
    for (int i = 0; i < MAX_ENEM; i++) {
        enemy_t *e = &G.enem[i];
        if (!e->alive) continue;
        e->t++;
        if (e->flash) e->flash--;
        enemy_behave(e);
        e->b.x += e->b.vx;
        e->b.y += e->b.vy;

        const sprite_t *s = enemy_sprite(e->type);
        int hw = s->w / 2, hh = s->h / 2;
        int x = FP_INT(e->b.x), y = FP_INT(e->b.y);

        if (y > FB_H + hh) { e->alive = false; continue; }
        if ((e->type == EN_ROID_L || e->type == EN_ROID_S) &&
            (x < -hw - 2 || x > FB_W + hw + 2)) {
            e->alive = false;
            continue;
        }

        if (hit_player(x, y, hw, hh)) {
            player_hit();
            enemy_kill(e, false);
            continue;
        }

        for (int j = 0; j < MAX_PBUL; j++) {
            pbullet_t *b = &G.pbul[j];
            if (!b->alive) continue;
            int bx = FP_INT(b->b.x), by = FP_INT(b->b.y);
            if (bx > x - hw && bx < x + hw && by > y - hh && by < y + hh) {
                b->alive = false;
                e->hp--;
                e->flash = 3;
                if (e->hp <= 0) {
                    enemy_kill(e, true);
                } else {
                    audio_play(SFX_HIT);
                    fx_sparks(bx, by, C_WHITE, 2);
                }
                break;
            }
        }
    }
}

void enemies_render(void) {
    for (int i = 0; i < MAX_ENEM; i++) {
        enemy_t *e = &G.enem[i];
        if (!e->alive) continue;
        const sprite_t *s = enemy_sprite(e->type);
        int x = FP_INT(e->b.x) - s->w / 2, y = FP_INT(e->b.y) - s->h / 2;
        if (e->flash)
            gfx_sprite_tint(x, y, s, C_WHITE);
        else
            gfx_sprite(x, y, s);
    }
}

// --- Pickups ---

static void apply_pickup(pu_t kind) {
    player_t *pl = &G.pl;
    switch (kind) {
        case PU_SPREAD: pl->spread_t = POWERUP_FRAMES; break;
        case PU_RAPID:  pl->rapid_t = POWERUP_FRAMES; break;
        case PU_SHIELD: pl->shield = true; break;
        case PU_BOMB:   if (pl->bombs < 4) pl->bombs++; break;
        case PU_LIFE:   if (pl->lives < 5) pl->lives++; break;
        default: break;
    }
    if (kind == PU_LIFE) {
        audio_play(SFX_LIFE);
        ledfx_event(LED_LIFE);
    } else if (kind == PU_SHIELD) {
        audio_play(SFX_POWERUP);
        ledfx_event(LED_SHIELD);
    } else {
        audio_play(SFX_POWERUP);
        ledfx_event(LED_POWERUP);
    }
    add_score(25);
}

void pickups_update(void) {
    for (int i = 0; i < MAX_PUPS; i++) {
        pickup_t *p = &G.pups[i];
        if (!p->alive) continue;
        p->b.y += p->b.vy;
        int x = FP_INT(p->b.x), y = FP_INT(p->b.y);
        if (y > FB_H + 5) { p->alive = false; continue; }
        if (hit_player(x, y, 5, 5)) {
            p->alive = false;
            apply_pickup(p->kind);
        }
    }
}

void pickups_render(void) {
    static const sprite_t *icons[PU_COUNT];
    icons[PU_SPREAD] = &spr_pu_spread;
    icons[PU_RAPID]  = &spr_pu_rapid;
    icons[PU_SHIELD] = &spr_pu_shield;
    icons[PU_BOMB]   = &spr_pu_bomb;
    icons[PU_LIFE]   = &spr_pu_life;
    for (int i = 0; i < MAX_PUPS; i++) {
        pickup_t *p = &G.pups[i];
        if (!p->alive) continue;
        if (G.frame & 16 && (G.frame & 3) == 0) continue; // shimmer
        const sprite_t *s = icons[p->kind];
        gfx_sprite(FP_INT(p->b.x) - s->w / 2, FP_INT(p->b.y) - s->h / 2, s);
    }
}
