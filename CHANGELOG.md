# Changelog

All notable changes to NOVA RAID are documented here. The format follows
[Keep a Changelog](https://keepachangelog.com/en/1.1.0/) and the project uses
[Semantic Versioning](https://semver.org/).

## [1.0.0] - 2026-07-10

Initial release.

### Added
- Complete arcade space shooter for the Raspberry Pi Pico W on the 52Pi Pico
  Breadboard Kit Plus (EP-0172): splash, menu, gameplay, pause, game over,
  initials entry, hall of fame, diagnostics.
- Four enemy behaviours (drone, darter, sentry, splitting asteroid) with
  per-wave difficulty scaling and four spawn patterns.
- Three-phase Void Dreadnought boss every fifth wave with scaling HP.
- Nova bomb secondary weapon, five power-ups, combo score multiplier.
- Dual-core renderer: 240×160 RGB565 framebuffer pixel-doubled to the
  480×320 ST7796 panel over 62.5 MHz SPI with double-buffered DMA, 25 fps.
- Priority-based buzzer sound sequencer (PWM) with effects and jingles.
- WS2812 lighting engine (base modes + event overlays) and panel LED
  indicators for bomb-ready / low-health.
- Top-5 high-score table with 3-letter callsigns persisted in the last flash
  sector, safe against reflashes.
- Hardware diagnostics screen (ADC, buttons, RGB, LEDs, buzzer sweep, colour
  bars, orientation arrow).
- Host-side harness building the identical game core for automated
  full-state-machine runs and frame capture; CI builds firmware + harness.
- Documentation: hardware identification, pin map, logical schematic, block
  diagram, architecture, build/flash for the three major OSes,
  troubleshooting, first-run verification.
