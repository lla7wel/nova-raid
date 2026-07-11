# Builds the frame-capture harness with the host compiler.
# Run from this directory:  make && ./capture <outdir>

SRC := ../../src
CC  ?= cc
CFLAGS := -O2 -Wall -Wextra -I$(SRC)

GAME_SRCS := $(SRC)/gfx/gfx.c $(SRC)/gfx/sprites.c \
             $(SRC)/game/game.c $(SRC)/game/entities.c $(SRC)/game/waves.c \
             $(SRC)/game/boss.c $(SRC)/game/starfield.c $(SRC)/game/hud.c \
             $(SRC)/game/audio_seq.c $(SRC)/game/ledfx.c \
             $(SRC)/game/hiscore.c $(SRC)/game/diag.c

capture: main.c hal_host.c $(GAME_SRCS)
	$(CC) $(CFLAGS) -o $@ $^

clean:
	rm -f capture
