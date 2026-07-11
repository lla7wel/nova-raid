## What

<!-- One-paragraph summary of the change. -->

## Validation

- [ ] `ninja -C build` produces a UF2 with no new warnings
- [ ] `make -C tools/host && ./tools/host/capture /tmp/frames` completes
- [ ] Tested on physical hardware (state board revision) — or explicitly noted as harness-only
- [ ] Sprite changes made in `tools/gen_sprites.py` and regenerated, if applicable
- [ ] Docs updated if behaviour, pins or controls changed
