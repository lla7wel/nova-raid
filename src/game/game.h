#ifndef NOVA_GAME_H
#define NOVA_GAME_H

// Game core, platform-independent: only hal.h and gfx.h may be included
// from here down so the same sources build for the Pico and the host
// frame-capture harness.

#include <stdint.h>
#include <stdbool.h>
#include "hal/hal.h"
#include "gfx/gfx.h"

// Positions and velocities are 24.8 fixed point (the RP2040 has no FPU).
#define FP(v)      ((int32_t)((v) * 256))
#define FP_INT(v)  ((int)((v) >> 8))

typedef enum {
    ST_SPLASH, ST_MENU, ST_HISCORES, ST_DIAG,
    ST_PLAY, ST_PAUSE, ST_GAMEOVER, ST_ENTRY,
} state_t;

typedef enum {
    EN_NONE, EN_DRONE, EN_DARTER, EN_SENTRY, EN_ROID_L, EN_ROID_S,
} etype_t;

typedef enum { PU_SPREAD, PU_RAPID, PU_SHIELD, PU_BOMB, PU_LIFE, PU_COUNT } pu_t;

#define MAX_PBUL 12
#define MAX_EBUL 28
#define MAX_ENEM 14
#define MAX_PUPS 4
#define MAX_FX   20

#define PLAYFIELD_TOP 12   // rows above are the HUD bar

typedef struct { int32_t x, y, vx, vy; } body_t;

typedef struct { body_t b; bool alive; } pbullet_t;
typedef struct { body_t b; bool alive; } ebullet_t;

typedef struct {
    etype_t type;
    body_t b;
    int16_t hp;
    uint16_t t;      // frames since spawn
    int16_t p0, p1;  // behaviour params (sine origin, dive state, ...)
    uint8_t flash;   // hit-flash frames remaining
    bool alive;
} enemy_t;

typedef struct { pu_t kind; body_t b; bool alive; } pickup_t;

// One slot renders either an explosion animation or a pixel spark.
typedef struct {
    body_t b;
    uint8_t age, life;
    uint16_t color;  // 0 = explosion sprite animation, else spark pixel
    bool alive;
} fx_t;

typedef struct {
    int32_t x, y;
    uint8_t lives, bombs;
    bool shield;
    uint8_t inv;             // invulnerability frames after a hit
    uint8_t fire_cd;
    uint16_t rapid_t, spread_t;
} player_t;

typedef struct {
    uint8_t phase;           // 0 sweep, 1 spawner, 2 barrage
    int16_t hp, hp_max;
    body_t b;
    uint16_t t;
    uint8_t flash;
    bool active;
} boss_t;

#define HISCORE_N 5
typedef struct {
    char name[HISCORE_N][4];
    uint32_t score[HISCORE_N];
} hiscore_table_t;

typedef struct {
    state_t state;
    uint32_t frame;          // frames in current state
    input_t in;

    player_t pl;
    pbullet_t pbul[MAX_PBUL];
    ebullet_t ebul[MAX_EBUL];
    enemy_t enem[MAX_ENEM];
    pickup_t pups[MAX_PUPS];
    fx_t fx[MAX_FX];
    boss_t boss;

    uint32_t score;
    uint8_t combo;           // score multiplier x1..x5
    uint16_t combo_t;

    uint8_t wave;
    uint8_t wave_phase;      // 0 banner, 1 spawning, 2 clearing, 3 done
    uint16_t wave_t;
    uint8_t spawn_left;
    uint8_t pattern;

    int menu_sel;
    uint32_t score_at_entry; // pending high-score entry
    int entry_pos;
    char entry_name[4];

    hiscore_table_t hi;
} game_t;

extern game_t G;

// game.c
void game_init(void);
void game_tick(void);        // one 40 ms fixed-timestep update + render
void game_enter(state_t s);

// entities.c
void play_reset(void);
void player_update(void);
void player_render(void);
void player_hit(void);
void bullets_update(void);
void bullets_render(void);
void enemies_update(void);
void enemies_render(void);
void pickups_update(void);
void pickups_render(void);
void fx_update(void);
void fx_render(void);
void fx_explosion(int x, int y);
void fx_sparks(int x, int y, uint16_t color, int n);
bool spawn_enemy(etype_t type, int x, int y);
void enemy_fire(int x, int y, int32_t speed_fp);
void enemy_kill(enemy_t *e, bool scored);
void add_score(uint32_t base);
void use_bomb(void);

// waves.c
void wave_start(uint8_t wave);
void wave_update(void);
bool wave_is_boss(uint8_t wave);
int  wave_speed_pct(void);   // 100 = base speed, grows with wave

// boss.c
void boss_start(void);
void boss_update(void);
void boss_render(void);
void boss_damage(int dmg);

// starfield.c
void starfield_init(void);
void starfield_update(int speed_pct);
void starfield_render(void);

// hud.c
void hud_render(void);
void banner_render(void);

// hiscore.c
void hiscore_load(void);
void hiscore_save(void);
bool hiscore_qualifies(uint32_t score);
void hiscore_insert(const char *name, uint32_t score);
uint32_t hiscore_best(void);

// diag.c
void diag_update(void);
void diag_render(void);

// audio_seq.c
typedef enum {
    SFX_NONE, SFX_MENU_MOVE, SFX_MENU_SEL, SFX_LASER, SFX_HIT,
    SFX_EXPLODE, SFX_BIG_EXPLODE, SFX_PLAYER_HIT, SFX_POWERUP,
    SFX_BOMB, SFX_LIFE, SFX_WAVE, SFX_BOSS_WARN, SFX_JINGLE_SPLASH,
    SFX_JINGLE_OVER, SFX_JINGLE_VICTORY, SFX_PAUSE,
} sfx_id_t;
void audio_play(sfx_id_t id);
void audio_update(void);     // call once per frame
void audio_stop(void);

// ledfx.c
typedef enum {
    LED_OFF, LED_MENU, LED_PLAY, LED_DAMAGE, LED_POWERUP, LED_SHIELD,
    LED_BOMB, LED_BOSS, LED_VICTORY, LED_GAMEOVER, LED_LIFE,
} led_event_t;
void ledfx_set(led_event_t base);   // persistent mode
void ledfx_event(led_event_t ev);   // transient overlay
void ledfx_update(void);

#endif
