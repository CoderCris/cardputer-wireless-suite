<div align="center">

# Cardputer Wireless Suite

**An open-source security multitool firmware for the M5Stack Cardputer V1.**
Wi-Fi, BLE and IR capture, built piece by piece, with the hardware explained.

![Board](https://img.shields.io/badge/board-M5Stack%20Cardputer%20V1-orange)
![SoC](https://img.shields.io/badge/SoC-ESP32--S3FN8-red)
![Build](https://img.shields.io/badge/build-PlatformIO-blue)
![Status](https://img.shields.io/badge/status-early%20development-yellow)
![License](https://img.shields.io/badge/license-MIT-green)

[Wiki](https://github.com/CoderCris/cardputer-wireless-suite/wiki) ·
[Roadmap](https://github.com/CoderCris/cardputer-wireless-suite/wiki/Roadmap) ·
[Hardware notes](https://github.com/CoderCris/cardputer-wireless-suite/wiki/Hardware-Overview) ·
[Open questions](https://github.com/CoderCris/cardputer-wireless-suite/wiki/Open-Questions)

</div>

---

## What this is

A Flipper-Zero-style toolbox for a pocket-sized ESP32-S3 computer, written for **blue team
and educational use**: the point is to see how passive capture works from the
inside, so it can be recognised and defended against.

Two things set it apart from flashing an existing firmware:

- **Every piece is small and explained.** Each tool is built as a sequence of
  flashable steps, and the code comments say what happens on the bus, in the
  peripheral or in the radio, not just what the line does.
- **Every hardware fact says where it comes from.** The wiki tags each claim as
  observed on a device, read from library source, or taken from a datasheet.
  Several widely repeated "facts" about this board turned out to be wrong.

## Status

Early. "Compiled" means it builds and links. "Verified" means it was flashed and
observed on a real Cardputer V1. Nothing is called working until it has been
observed.

| Tool | What it does | Status |
|------|--------------|--------|
| Interactive terminal | Keyboard in, display out, line editing with wrap | Verified on hardware |
| MicroSD logger | Mounts the card over SPI, boot report with failure hints | Compiled, not yet verified with a card |
| Wi-Fi sniffer | Promiscuous mode, frame counters by type on a fixed channel | Piece 1 of 5 compiled, not yet verified |
| IR transmitter | Send known codes through the built-in IR LED | Planned |
| BLE scanner | Passive advertising capture | Planned |
| IR capture and replay | Needs an external receiver on the Grove port | Blocked on hardware |
| Audio side channel | PDM microphone analysis | Planned, experimental |
| Navigator | Browse what the tools captured | Planned |

The order, and why each piece depends on the previous ones, is in the
[Roadmap](https://github.com/CoderCris/cardputer-wireless-suite/wiki/Roadmap).

## Things worth knowing about the Cardputer V1

Found while building this, and documented with sources in the wiki:

- **There is no PSRAM.** The chip is an ESP32-S3**FN8**: 8 MB of flash, no PSRAM.
  The "8 MB PSRAM" you may have read is the flash. Design for about 512 KB of
  internal SRAM.
- **A 16 MB partition table bricks the boot, silently.** It compiles, flashes,
  and then resets about 40 times per second with a black screen and no log.
  Use `default_8MB.csv`.
- **Display and SD card share no pins.** The often-cited GPIO12 conflict comes
  from a wrong pin table. They do use up both general-purpose SPI controllers.
- **There is no IR receiver.** GPIO46 is the microphone data line. The only IR
  part is the transmitter on GPIO44.
- **The Grove port is the only expansion point**: two GPIO.

## Build and flash

You need [PlatformIO](https://platformio.org/) and a Cardputer V1 on USB-C.

```sh
git clone https://github.com/CoderCris/cardputer-wireless-suite.git
cd cardputer-wireless-suite
pio run               # build
pio run -t upload     # flash
pio device monitor    # serial output over the native USB
```

If the upload fails, enter the bootloader by hand: hold **BOOT** (G0), press and
release **RESET**, release BOOT. More in
[PlatformIO Setup](https://github.com/CoderCris/cardputer-wireless-suite/wiki/PlatformIO-Setup).

> The Cardputer **ADV** has a different keyboard (I2C) and pinout. This firmware
> targets the **V1** only.

## Repository layout

```
src/        firmware: one module per tool, wired together in main.cpp
docs/       design and hardware reference (Spanish)
sandbox/    small host-side C++ experiments, not part of the firmware
```

Development happens on one branch per milestone (`hito-NN-…`); `main` receives
them as they are verified.

## Documentation

| Where | Language | What |
|-------|----------|------|
| [Wiki](https://github.com/CoderCris/cardputer-wireless-suite/wiki) | English | Hardware facts with provenance, roadmap, open questions |
| [`docs/`](docs/) | Spanish | Working reference: protocols, data model, per-milestone journals |

The Spanish docs are the source; the wiki is updated from them.

## Contributing

The most valuable contribution is an **observation on real hardware**: flash a
branch, and open an issue saying what the screen and the serial monitor showed,
and on which unit. Answers to the
[open questions](https://github.com/CoderCris/cardputer-wireless-suite/wiki/Open-Questions),
especially anything that needs the official schematic, are just as welcome. If
your device contradicts something written here, the device wins.

## Responsible use

The capture tools are passive. Anything that transmits is kept behind an
explicit step in the interface. Monitoring or interfering with networks and
devices you do not own or have permission to test is illegal in most places.
You are responsible for how you use this firmware.

## License

[MIT](LICENSE). Use it, change it and ship it, as long as the copyright notice
stays with it. No warranty.
