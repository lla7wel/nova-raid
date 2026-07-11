# Building and flashing

## The easy path (no toolchain)

Grab `nova_raid.uf2` from the [latest release](../../../releases/latest),
hold **BOOTSEL** while plugging the Pico in, and drop the file onto the
`RPI-RP2` drive. Done — skip to [First-run verification](#first-run-verification).

## Prerequisites for building

| Tool | Version | Notes |
|---|---|---|
| CMake | ≥ 3.13 | |
| Ninja or GNU Make | any recent | |
| Arm GNU toolchain | 13+ (`arm-none-eabi-gcc` **with newlib**) | see per-OS notes |
| Python 3 | ≥ 3.8 | asset generators only |
| Pico SDK | **2.1.1** (tested) | with the `tinyusb` submodule |
| Git | any recent | |

### Get the SDK (all platforms)

```sh
git clone https://github.com/raspberrypi/pico-sdk --branch 2.1.1 --depth 1
git -C pico-sdk submodule update --init --depth 1 lib/tinyusb
export PICO_SDK_PATH="$PWD/pico-sdk"     # Windows PowerShell: $env:PICO_SDK_PATH="$PWD\pico-sdk"
```

### Toolchain per OS

- **Linux (Debian/Ubuntu):** `sudo apt install cmake ninja-build gcc-arm-none-eabi libnewlib-arm-none-eabi python3`
- **macOS:** `brew install cmake ninja` plus the
  [Arm GNU toolchain](https://developer.arm.com/downloads/-/arm-gnu-toolchain-downloads)
  or the [xPack build](https://github.com/xpack-dev-tools/arm-none-eabi-gcc-xpack/releases)
  (unpack anywhere, add its `bin/` to `PATH`). Homebrew's bare
  `arm-none-eabi-gcc` formula lacks newlib and will fail with
  `cannot read spec file 'nosys.specs'`.
- **Windows:** install the
  [Raspberry Pi Pico VS Code extension](https://github.com/raspberrypi/pico-vscode)
  (bundles everything), or CMake + Ninja + the Arm GNU toolchain installer,
  then build from a "Developer" shell where all three are on `PATH`.

## Build

```sh
cmake -S . -B build -G Ninja -DCMAKE_BUILD_TYPE=Release
ninja -C build
```

Outputs land in `build/`: `nova_raid.uf2` (flash this), plus `.elf`/`.map`
for debugging. For a non-wireless Pico add `-DPICO_BOARD=pico`.

If you edit the sprite art or want to regenerate the README banner:

```sh
python3 tools/gen_sprites.py      # rewrites src/gfx/sprites.{h,c}
python3 tools/gen_banner.py      # rewrites docs/images/banner.png (needs Pillow)
```

## Flashing options

1. **UF2 drag-and-drop** (recommended): hold BOOTSEL, plug in, copy
   `build/nova_raid.uf2` to `RPI-RP2`.
2. **picotool:** `picotool load -f build/nova_raid.uf2 && picotool reboot`
3. **SWD** (Debug Probe): `openocd -f interface/cmsis-dap.cfg -f target/rp2040.cfg -c "program build/nova_raid.elf verify reset exit"`

## First-run verification

1. Power on → NOVA RAID splash with starfield and jingle.
2. Press BTN1 → menu. Move the joystick: the selection follows and blips.
3. Open **DIAGNOSTICS** and check, in order:
   - ADC values move across roughly 0–4095 on both axes; the green dot
     follows the stick, and pushing the stick **up** moves the dot **up**.
   - BTN1/BTN2 rows show `[X]` while held.
   - The RGB LED cycles red → green → blue → white; panel LEDs D1/D2
     alternate.
   - Hold BTN1: rising tone sweep from the buzzer.
   - The colour bars are distinct and the yellow arrow points **up**.
   - Hold both buttons ~1 s to exit.
4. Start a mission; fire, bomb, pause (both buttons), resume, and let the
   ship be destroyed three times — the game-over path, initials entry (if you
   beat a default score) and hall of fame should all work.
5. Power-cycle: your score survives in the hall of fame.

The firmware also logs a boot line over USB CDC (any serial terminal,
115200 8N1) for confirming it is alive with the display disconnected.

Anything off? → [troubleshooting.md](troubleshooting.md).

## Validation

What is verified automatically and what was verified for the v1.0.0 release:

| Check | How | Status |
|---|---|---|
| Firmware compiles clean (UF2 produced) | CI on every push (SDK 2.1.1, arm-none-eabi 13) | ✅ |
| Game logic runs every state without crashing | CI builds `tools/host` harness and drives splash→menu→play→boss→pause→game-over→entry→hall-of-fame→diagnostics | ✅ |
| Screenshots reflect real rendering | captured from the harness, not mocked up | ✅ |
| RAM budget | `arm-none-eabi-size`: ~165 KiB static of 264 KiB | ✅ |
| Pin map matches vendor documentation | cross-checked against 52Pi wiki EP-0172 + GeeekPi demo sources | ✅ |
| Runs on physical EP-0172 | **not performed** — no board access during development | ⚠️ see below |

Physical checks that remain for the first person with the board in hand
(expected result, and the fallback if it fails):

- Panel initialises with the ST7796 sequence → else the lot uses another
  controller ([troubleshooting](troubleshooting.md#display-blank-or-garbled)).
- Rotation/joystick orientation match the defaults → else flip the
  documented `config.h` flags.
- SPI stable at 62.5 MHz → else halve `LCD_BAUD_HZ`.
