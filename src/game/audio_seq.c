#include "game/game.h"

// Single-channel note sequencer for the passive buzzer. A playing effect
// is preempted only by an equal or higher priority one; note duration is
// consumed in 40 ms frame steps, so durations are multiples of the frame.

typedef struct { uint16_t f; uint16_t ms; } note_t;  // f=0 is a rest

typedef struct {
    const note_t *notes;
    uint8_t len;
    uint8_t prio;
    uint8_t vol;
} seq_t;

#define SEQ(name) static const note_t name[]

SEQ(sq_menu_move) = { {1200, 30} };
SEQ(sq_menu_sel)  = { {900, 40}, {1400, 60} };
SEQ(sq_laser)     = { {1900, 20}, {1200, 20} };
SEQ(sq_hit)       = { {300, 30} };
SEQ(sq_explode)   = { {700, 40}, {350, 40}, {180, 60} };
SEQ(sq_big_expl)  = { {900, 40}, {500, 60}, {250, 80}, {120, 120} };
SEQ(sq_player_hit)= { {200, 60}, {0, 30}, {150, 90} };
SEQ(sq_powerup)   = { {800, 40}, {1100, 40}, {1500, 60} };
SEQ(sq_bomb)      = { {1800, 30}, {1400, 30}, {900, 40}, {400, 80}, {150, 120} };
SEQ(sq_life)      = { {1000, 60}, {1300, 60}, {1700, 60}, {2100, 100} };
SEQ(sq_wave)      = { {900, 60}, {0, 30}, {900, 60}, {1350, 100} };
SEQ(sq_boss_warn) = { {600, 100}, {0, 60}, {600, 100}, {0, 60}, {600, 100},
                      {0, 60}, {450, 200} };
SEQ(sq_splash)    = { {660, 90}, {880, 90}, {990, 90}, {1320, 180}, {0, 60},
                      {990, 90}, {1320, 240} };
SEQ(sq_over)      = { {880, 120}, {830, 120}, {780, 120}, {520, 300} };
SEQ(sq_victory)   = { {1050, 80}, {1320, 80}, {1580, 80}, {2100, 160}, {0, 40},
                      {1580, 80}, {2100, 240} };
SEQ(sq_pause)     = { {1400, 40}, {700, 40} };

#define E(sq, prio, vol) { sq, sizeof(sq) / sizeof(note_t), prio, vol }

static const seq_t table[] = {
    [SFX_NONE]           = { 0, 0, 0, 0 },
    [SFX_MENU_MOVE]      = E(sq_menu_move, 1, 120),
    [SFX_MENU_SEL]       = E(sq_menu_sel, 2, 150),
    [SFX_LASER]          = E(sq_laser, 1, 90),
    [SFX_HIT]            = E(sq_hit, 1, 110),
    [SFX_EXPLODE]        = E(sq_explode, 2, 160),
    [SFX_BIG_EXPLODE]    = E(sq_big_expl, 4, 210),
    [SFX_PLAYER_HIT]     = E(sq_player_hit, 5, 220),
    [SFX_POWERUP]        = E(sq_powerup, 3, 150),
    [SFX_BOMB]           = E(sq_bomb, 5, 220),
    [SFX_LIFE]           = E(sq_life, 4, 180),
    [SFX_WAVE]           = E(sq_wave, 3, 150),
    [SFX_BOSS_WARN]      = E(sq_boss_warn, 5, 200),
    [SFX_JINGLE_SPLASH]  = E(sq_splash, 6, 160),
    [SFX_JINGLE_OVER]    = E(sq_over, 6, 190),
    [SFX_JINGLE_VICTORY] = E(sq_victory, 6, 190),
    [SFX_PAUSE]          = E(sq_pause, 4, 140),
};

static const seq_t *cur;
static int pos;
static int remain_ms;

void audio_play(sfx_id_t id) {
    const seq_t *s = &table[id];
    if (!s->notes) return;
    if (cur && pos < cur->len && s->prio < cur->prio) return;
    cur = s;
    pos = -1;
    remain_ms = 0;
}

void audio_stop(void) {
    cur = 0;
    hal_tone(0, 0);
}

void audio_update(void) {
    if (!cur) return;
    remain_ms -= 1000 / GAME_FPS;
    if (remain_ms > 0) return;
    pos++;
    if (pos >= cur->len) {
        audio_stop();
        return;
    }
    const note_t *n = &cur->notes[pos];
    remain_ms += n->ms;
    if (remain_ms <= 0) remain_ms = n->ms;
    hal_tone(n->f, n->f ? cur->vol : 0);
}
