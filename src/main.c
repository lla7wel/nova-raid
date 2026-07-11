#include "pico/stdlib.h"
#include "pico/multicore.h"

#include "config.h"
#include "game/game.h"
#include "hal/st7796.h"

#include <stdio.h>

// Core 0 simulates and renders into one framebuffer while core 1 streams
// the other to the panel (the SPI push dominates the frame budget, see
// docs/architecture.md). Buffers swap once both sides finish.
//
// The handoff is a plain mailbox variable rather than the inter-core
// FIFO: the SDK's multicore_lockout, required while the high-score sector
// is flashed, owns the FIFO.

void hal_pico_init(void);

static uint16_t fb[2][FB_W * FB_H];
static uint16_t *volatile pending_frame;

static void core1_main(void) {
    // Lets core 0 freeze this core during flash writes.
    multicore_lockout_victim_init();
    while (true) {
        uint16_t *frame = pending_frame;
        if (!frame) {
            tight_loop_contents();
            continue;
        }
        st7796_push_frame(frame);
        pending_frame = NULL;
    }
}

int main(void) {
    stdio_init_all();
    hal_pico_init();
    st7796_init();

    multicore_launch_core1(core1_main);

    game_init();

    int cur = 0;
    absolute_time_t next = get_absolute_time();

    printf("NOVA RAID up: %ux%u fb, %u fps target\n", FB_W, FB_H, GAME_FPS);

    while (true) {
        gfx_set_target(fb[cur]);
        game_tick();

        while (pending_frame) tight_loop_contents();
        pending_frame = fb[cur];
        cur ^= 1;

        next = delayed_by_ms(next, 1000 / GAME_FPS);
        sleep_until(next);
    }
}
