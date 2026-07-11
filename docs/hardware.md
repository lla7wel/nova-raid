# Hardware

## Board identification

The project targets the hardware in the owner-supplied reference photograph:
a black expansion board silk-screened **"Pico Breadboard Kit"** with a
**Raspberry Pi Pico W** (silk "Raspberry Pi Pico W © 2022") seated in its
headers, a large TFT with a **"TFT Controller"** silk block, a PSP-style
joystick (silk "Joystick", "ADC0"/"ADC1"), an "RGB Led" position, two blue
push buttons ("Button"), a piezo buzzer, and a 4-LED block (D1–D4 with
"GP16 / GP17 / 3V3 / 5V" markings).

That combination matches the **52Pi / GeeekPi Pico Breadboard Kit Plus**,
product id **EP-0172** ("Plus version" on the silk), documented at the
[52Pi wiki](https://wiki.52pi.com/index.php?title=EP-0172) with demo firmware
at [geeekpi/pico_breadboard_kit](https://github.com/geeekpi/pico_breadboard_kit).

| Property | Value | Confidence |
|---|---|---|
| Expansion board | 52Pi Pico Breadboard Kit Plus (EP-0172) | High — silk screen + component layout match |
| MCU board | Raspberry Pi Pico W (RP2040, 2 MB flash, CYW43439 radio unused) | Confirmed from photo silk screen |
| Display | 3.5" 480×320 TFT, **ST7796S** (marketed as ST7796SU1), SPI | High — vendor wiki & demo code; *controller may vary by production lot* |
| Touch | Capacitive, I²C (FT6236-class) | Vendor docs; unused by this project |
| Joystick | 2-axis analog (2 potentiometers) | Confirmed by "ADC0/ADC1" silk |
| Buttons | 2 momentary, active-low | Vendor docs |
| Buzzer | Piezo, GPIO/PWM driven | Vendor docs |
| RGB LED | Single WS2812 | Vendor docs |

## Pin map

Everything is hard-wired on the kit; the table is the single source of truth
mirrored by [`src/config.h`](../src/config.h).

| GPIO | Function | Peripheral | Direction | Notes |
|---|---|---|---|---|
| GP2 | SPI0 SCK | TFT | out | 62.5 MHz |
| GP3 | SPI0 MOSI | TFT | out | |
| GP4 | SPI0 MISO | TFT | in | wired, unused |
| GP5 | GPIO | TFT CS | out | active low |
| GP6 | GPIO | TFT DC | out | 0 = command |
| GP7 | GPIO | TFT RST | out | active low |
| GP8 | I2C0 SDA | touch | — | unused |
| GP9 | I2C0 SCL | touch | — | unused |
| GP12 | PIO0 | WS2812 RGB LED | out | 800 kHz |
| GP13 | PWM | buzzer | out | tone + duty volume |
| GP14 | GPIO | BTN2 (bomb/back) | in | active low, pull-up |
| GP15 | GPIO | BTN1 (fire/confirm) | in | active low, pull-up |
| GP16 | GPIO | LED D1 | out | bomb-ready indicator |
| GP17 | GPIO | LED D2 | out | low-health blink |
| GP26 | ADC0 | joystick X | in | |
| GP27 | ADC1 | joystick Y | in | |

LEDs D3 and D4 on the kit are tied to the 3V3 and 5V rails (power indicators)
and are not software controllable. UART pins GP0/GP1 are unused; stdio goes
over USB CDC.

## Diagrams

- [Hardware block diagram](diagrams/block-diagram.svg)
- [Logical schematic](diagrams/schematic.svg)
- [Controls](diagrams/controls.svg)

The SVGs are the editable sources (plain, hand-written SVG).

## Electrical safety notes

- All GPIO signalling is 3.3 V; the firmware never configures a pin against
  the kit's wiring (inputs stay inputs, outputs stay outputs).
- The WS2812 is brightness-capped in software (`RGB_MAX_BRIGHT`) — full white
  at point-blank range is unpleasant, and the cap keeps current draw trivial.
- The SPI clock (62.5 MHz) is at the edge of the panel's tested range
  (vendor demo: 60 MHz). If a particular panel shows corruption, halve
  `LCD_BAUD_HZ`; nothing can be damaged by the higher clock, it simply
  wouldn't display correctly.
- Flash writes are confined to the last 4 KiB sector, far above the ~120 KiB
  firmware image, with core 1 parked and interrupts off during programming.

## Configurable assumptions

These could not be physically verified from the photograph alone and may vary
between board lots. Each is one line in [`src/config.h`](../src/config.h),
and the **Diagnostics** screen (main menu → DIAGNOSTICS) shows exactly which
one to change:

| Assumption | Default | If wrong | Config |
|---|---|---|---|
| Panel rotation | landscape, joystick left (`LCD_ROTATION 1`) | picture upside down → set `3`; arrow in diagnostics must point up | `LCD_ROTATION` |
| Display controller | ST7796S | blank/garbled screen on some lots (ILI9488 variants need a different init + 18-bit colour; see [troubleshooting](troubleshooting.md)) | `st7796.c` init table |
| Joystick X direction | right = increase | ship moves opposite to stick | `JOY_INVERT_X` |
| Joystick Y direction | inverted (`JOY_INVERT_Y 1`) | up/down swapped | `JOY_INVERT_Y` |
| Axis assignment | X=ADC0, Y=ADC1 | diagonal confusion | `JOY_SWAP_AXES` |
| Button silk mapping | BTN1=GP15, BTN2=GP14 | fire and bomb swapped | `PIN_BTN_FIRE` / `PIN_BTN_ALT` |

Confirmed facts (photo or vendor doc) vs. inferences are labelled in the
schematic legend; the buzzer's on-board driver stage is inferred, which does
not affect the firmware (it only drives GP13 with PWM either way).
