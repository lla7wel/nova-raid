# Contributing

Thanks for your interest in NOVA RAID. Small, focused pull requests are very
welcome — especially reports and fixes from people who run it on a physical
EP-0172 kit.

## Ground rules

- One change per PR; describe what you tested and on what hardware (or note
  that you validated through the host harness only).
- Keep the game core (`src/game/`, `src/gfx/`) free of Pico headers — it must
  keep building with `tools/host/Makefile`.
- All pin and tuning constants belong in `src/config.h`, not inline.
- Sprite changes go through `tools/gen_sprites.py` (edit the ASCII art, run
  the script, commit both); never hand-edit `src/gfx/sprites.{h,c}`.
- Match the existing style: C11, 4-space indent, lower_snake_case, comments
  only where the code can't speak for itself.

## Before submitting

```sh
# firmware must build
cmake -S . -B build -G Ninja && ninja -C build

# host harness must build and complete a full run
make -C tools/host && ./tools/host/capture /tmp/frames
```

CI runs both on every push; a green run is required to merge.

## Reporting hardware variance

Board lots differ (display controller, joystick orientation). If defaults are
wrong for your board, open a bug report with photos and the diagnostics-screen
readings — that information directly improves `docs/hardware.md`.
