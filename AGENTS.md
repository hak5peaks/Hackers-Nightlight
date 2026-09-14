# Agent notes: Hackers-Nightlight

Orientation for an AI agent picking up work in this repo. For product/marketing
framing, see `README.md`. This file is about the codebase and where things stand.

## What this repo is

Custom firmware (ESP32-C3) that turns a handful of commodity WiFi smart bulbs into
covert pentesting tools: WiFi deauth/PMKID/handshake-capture attacks plus the bulb's
normal RGB/white LED control, all served from a web UI hosted by the bulb's own AP.

## Repo layout

```
firmware/          Unified, config-driven build - ALL current firmware source lives here.
  src/, include/    Shared code for every bulb - WiFi attacks, web server, cJSON, LED
                    driver abstraction. No per-bulb source files.
  boards/           generate_board_config.py only (the config->header generator).
                    NOT board configs - those moved out, see below.
  test/             input_html.html (the one shared web UI template) + converter.py
                    (renders it + gzips it into a C header) + a couple of test scripts.
  app_config.json   Settings shared by every bulb (AP SSID/password, hidden or not).
  platformio.ini    One [env:<name>] per bulb, all pointing at the same src/include tree.
  README.md         The real reference: config generation, LED driver abstraction,
                    current status/open items. Read this before touching firmware/.

OREIN/, VONT/, WYZE/, RAZER/     One folder per supported bulb, at the repo root.
  board.json        That bulb's config (pins, LED driver strategy, quirks) - read by
                    firmware/boards/generate_board_config.py via `pio run -e <name>`.
                    This is the ONLY thing here that feeds the build.
  readme.md         Teardown/pinout/flashing notes specific to that physical bulb
                    (where one exists - not every bulb has one, e.g. VONT doesn't).
  images/           Teardown photos, where present. That's it for this folder - no
                    firmware source lives here anymore (see below).

.github/workflows/  firmware-build.yml builds every board on push/PR (uploads binaries
                    as artifacts); firmware-release.yml does the same on a `v*` tag and
                    attaches them to a GitHub Release. Both key off a `board:` matrix
                    list that needs a new bulb's env name added too.
.claude/skills/new-bulb/   Scaffolds a new bulb (board.json, platformio.ini env, CI
                    matrix entries). Use this instead of doing it by hand.
```

**If you're asked to add support for a new bulb**, use the `new-bulb` skill, or see
firmware/README.md's "Adding a new bulb" section - short version: new `<NAME>/` folder
+ `board.json`, one `platformio.ini` line, one line in each CI workflow's board matrix,
no firmware source changes for a normal bulb.

**If you're asked about LED color/white behavior, pin assignments, or the WiFi
attacks**, the logic is entirely in `firmware/src/`+`firmware/include/`, branched per
board at compile time via `#if`/`#elif` on macros generated from that board's
`board.json` - see `firmware/include/generated/board_config.h` after a build, or
`firmware/boards/generate_board_config.py` for how it's produced.

## Build/test

```
cd firmware
pio run -e orein            # or vont, wyze, razer
pio run -e orein -t upload --upload-port /dev/ttyACM1
python3 test/converter.py orein   # preview a board's rendered web UI HTML without a full build
```

`pio` (PlatformIO CLI) is installed in this environment - always verify firmware
changes build (ideally for all four boards) before calling a change done.

## Current status / open items

- Three branches in play: `main` (upstream, old fully-duplicated layout), `OREIN`
  (this fork's feature branch), and this worktree's branch (on top of `OREIN`, adding
  the `firmware/` consolidation). Nothing merged anywhere yet.
- OREIN's PWM2 (GPIO4) is confirmed to do nothing to the LED array (bench-tested
  2026-09-12) - not wired to the LED board's 6-pin connector at all. The old bench-test
  toggle and extra-PWM machinery have been removed from the codebase.
- VONT now shares OREIN's newer web UI (swatches, single-click select) - compatible
  with VONT's existing `/setcolor` contract, but worth a sanity check on real hardware.
- VONT/WYZE haven't been build-verified against real hardware through `firmware/` yet -
  only OREIN has. Their old per-bulb source is gone (see below), so verification now
  is a real flash+test, not a diff against the original.
- Legacy per-bulb firmware source (each folder's old `src/`, `include/`,
  `platformio.ini`, `.pio/`, `bins/`, `test/`) has been deleted - still in git history
  if ever needed. Each bulb folder now holds only `board.json` + docs.
