#include "game/game.h"

#include <string.h>

static const hiscore_table_t defaults = {
    .name  = { "ACE", "NOV", "RAI", "GPI", "P1 " },
    .score = { 5000, 4000, 3000, 2000, 1000 },
};

void hiscore_load(void) {
    if (!hal_store_load(&G.hi, sizeof G.hi))
        G.hi = defaults;
}

void hiscore_save(void) {
    hal_store_save(&G.hi, sizeof G.hi);
}

uint32_t hiscore_best(void) {
    return G.hi.score[0];
}

bool hiscore_qualifies(uint32_t score) {
    return score > G.hi.score[HISCORE_N - 1];
}

void hiscore_insert(const char *name, uint32_t score) {
    int pos = HISCORE_N;
    while (pos > 0 && score > G.hi.score[pos - 1]) pos--;
    if (pos >= HISCORE_N) return;
    for (int i = HISCORE_N - 1; i > pos; i--) {
        G.hi.score[i] = G.hi.score[i - 1];
        memcpy(G.hi.name[i], G.hi.name[i - 1], 4);
    }
    G.hi.score[pos] = score;
    memcpy(G.hi.name[pos], name, 3);
    G.hi.name[pos][3] = 0;
    hiscore_save();
}
