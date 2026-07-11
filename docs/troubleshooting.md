# Troubleshooting

Work through the sections in order — each assumes the previous ones pass.
The **DIAGNOSTICS** screen (main menu → third item) is the main tool here.

## The board doesn't enumerate / no RPI-RP2 drive

- Use a micro-USB **data** cable; many bundled cables are charge-only.
- Hold BOOTSEL *before* inserting the cable and release after the drive
  appears.
- If the game is already flashed, the Pico won't show as a drive — that's
  normal. Re-enter BOOTSEL to reflash.

## Nothing on the display

- Confirm the firmware is alive: connect a serial terminal to the Pico's USB
  CDC port — the game prints `NOVA RAID up: ...` at boot. If nothing prints,
  the flash didn't take; reflash the UF2.
- Reseat the Pico in the kit headers (all pins fully home, USB port facing
  the board edge marked on the silk).
- Make sure the board is the **Plus** (EP-0172) kit — the non-Plus EP-0164
  has a different display (ILI9341, different pins) and needs a port.

## Display blank or garbled

- Lower the SPI clock: in `src/config.h` set `LCD_BAUD_HZ` to `(31250*1000)`
  and rebuild. Some panels/lots don't tolerate 62.5 MHz.
- Colours inverted or tinted: production lots vary. Try commenting the
  inversion command in `st7796.c` (`cmd_n(0xB4, ...)`) or adding `cmd(0x21)`
  (INVON) after `cmd(0x29)`.
- Completely wrong image geometry (stripes, quarter screen): the lot may use
  an ILI9488 instead of the ST7796. ILI9488 needs a different init table and
  **18-bit colour over SPI** — open an issue with a photo; the driver is
  structured so an alternate init + pixel format can be added in `st7796.c`.

## Picture upside down or mirrored

- Set `LCD_ROTATION` to `3` (or try `0`/`2` for portrait experiments) in
  `src/config.h`. The diagnostics arrow must point at the physical top edge
  when the board is held with the joystick on the left.

## Ship moves the wrong way

- Diagnostics shows raw ADC and scaled values live. Flip `JOY_INVERT_X` /
  `JOY_INVERT_Y`, or set `JOY_SWAP_AXES 1` if left/right moves the dot
  vertically.
- Drifting without touching the stick: increase `JOY_DEADZONE` (default 300).
  Worn sticks can need 450+.

## Buttons swapped or dead

- Swapped: exchange `PIN_BTN_FIRE`/`PIN_BTN_ALT` in `src/config.h`.
- Dead: in diagnostics, a held button must show `[X]`. If not, the kit's
  button header may be damaged — the pins are wired directly, there is no
  configuration to fix.

## No sound / constant faint whine

- The buzzer is driven at 50 % max duty with volume-scaled pulse width; a
  faint idle whine should never occur (PWM is disabled between notes). If it
  does, reflash — a crashed core can leave PWM running.
- No sound at all: check diagnostics tone sweep while holding BTN1.

## RGB LED wrong colours or dead

- WS2812 order is GRB (handled in `hal_rgb`). If red and green appear
  swapped on your lot, exchange the `r`/`g` bytes there.
- Dead LED but everything else works: some kits populate the RGB LED
  differently; the game remains fully playable.

## High scores don't persist

- Scores save only when the initials entry completes (three letters
  confirmed with FIRE).
- Flashing *other* projects can erase the sector; NOVA RAID itself never
  touches it during reflash.
- A corrupted table self-heals: an invalid magic number falls back to
  defaults on next boot.

## Game runs but stutters

- Expected cadence is a locked 25 fps. Stutter usually means a debug/
  non-Release build (`-DCMAKE_BUILD_TYPE=Release`) or a lowered SPI clock —
  at 31.25 MHz the transfer alone takes ~79 ms (≈12 fps).
