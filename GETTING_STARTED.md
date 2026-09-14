# Getting started (development)

`README.md` covers the project's background and mission. This doc is the practical
guide to building, flashing, and extending the firmware now that it's been
consolidated into a single codebase - start here if you want to get a bulb flashed or
add a new one.

## Project structure

```
firmware/                  All current firmware source - one shared codebase, every
                            supported bulb builds from it.
  src/, include/           Shared code: WiFi attacks, web server, LED driver
                            abstraction, cJSON. No per-bulb source files live here.
  boards/                  generate_board_config.py - reads a bulb's board.json (below)
                            and turns it into a compiled-in C header before each build.
  test/                    input_html.html (the one shared web UI) + converter.py,
                            which renders and gzips it into the served page.
  app_config.json          Settings shared by every bulb: AP SSID/password, hidden or not.
  platformio.ini           One [env:<name>] per bulb, all building the same source tree.
  README.md                The full technical reference - config generation, LED driver
                            abstraction, current status. Read before non-trivial changes.

OREIN/, VONT/, WYZE/, RAZER/     One folder per supported bulb, at the repo root.
  board.json                That bulb's config: pins, LED driver strategy, quirks.
                             This is what `pio run -e <name>` actually reads.
  readme.md                 Teardown/pinout/flashing notes for that physical bulb
                             (where one exists - some bulbs don't have one, e.g. VONT).
  images/                   Teardown photos, where present.
```

No firmware source lives in these per-bulb folders - the old per-bulb copies (`src/`,
`include/`, `platformio.ini`, `.pio/`, `bins/`, `test/`) were deleted once `firmware/`
was confirmed to build all four boards. All firmware code lives in `firmware/` now.

## Supported bulbs

| Bulb | LED driver | Color control | Status |
|---|---|---|---|
| OREIN (`OREIN/`) | `bus` (BP6758-family chip) | Enabled | Working on real hardware; one open question (see firmware/README.md) |
| VONT (`VONT/`) | `pwm_direct` (native ESP32 PWM) | Enabled | Builds clean, not yet flash-tested on real hardware |
| WYZE (`WYZE/`) | `bus` (BP5758 chip) | Disabled (`led_control_enabled: false`) | Same driver code as OREIN, not yet verified live on WYZE hardware |
| RAZER (`RAZER/`) | `none` | Not supported | No teardown done yet - `board.json` is honest placeholders/TODOs |

## Prerequisites

- [PlatformIO Core](https://platformio.org/) (`pio` on your `PATH`). Everything below
  assumes it's installed - no Arduino IDE / PlatformIO IDE needed.
- A USB-to-UART adapter to flash over the bulb's exposed UART pads (see that bulb's own
  `readme.md` for its specific pin-out and boot-strap sequence).

## Build and flash a bulb

```
cd firmware
pio run -e orein                                          # build only - or vont, wyze, razer
pio run -e orein -t upload --upload-port /dev/ttyACM1      # build + flash
```

That's the whole workflow - `-e <name>` selects `../<NAME>/board.json` (upper-cased),
which drives every board-specific detail (pins, LED behavior, AP name). No source file
needs editing to build a different bulb.

Once flashed, the bulb starts its own AP (`Nightlight` / `Nightlight12345` by default,
see `firmware/app_config.json` - set `ap_hidden: true` there to stop broadcasting the
SSID) - connect to it and open `192.168.4.1` for the web UI.

## Adding a new bulb

Short version: create a `<NEWNAME>/` folder with a `board.json` (copy `OREIN/board.json`
as a starting point), add one `[env:<newname>]` line to `firmware/platformio.ini`, add
`<newname>` to the CI workflows' board matrix, then `pio run -e <newname>`. **Don't
invent pin numbers** - every pin fact in the existing configs came from continuity
checks + a live LED test on real hardware. If you're using Claude Code, the `new-bulb`
skill automates the scaffolding. Full details, including what to do if your board's LED
hardware doesn't fit the existing driver strategies, are in `firmware/README.md`.

## Where to look next

- `firmware/README.md` - the firmware architecture in full, plus current status/open items.
- `AGENTS.md` - a shorter orientation aimed at an AI coding agent.
- Each bulb's own `readme.md` (where present) - hardware-specific teardown, pinout, and
  flashing notes for that physical device.
