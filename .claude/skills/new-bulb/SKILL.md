---
name: new-bulb
description: Scaffold a new supported bulb for Hackers-Nightlight - creates <NAME>/board.json, adds the platformio.ini env, and wires it into CI. Use when a contributor wants to add support for a new smart bulb model.
---

# Add a new bulb to Hackers-Nightlight

Scaffolds everything needed for a new bulb to build alongside OREIN/VONT/WYZE/RAZER.
Read `firmware/README.md`'s "Adding a new bulb" and "Config reference" sections first -
this skill automates the mechanical steps, not the hardware investigation.

## Before touching any files

Ask the user (don't guess) for whatever of this isn't already known:

- Bulb name/model, and a short upper-case folder name (e.g. `GOVEE`) matching the
  existing `OREIN`/`VONT`/`WYZE`/`RAZER` convention.
- MCU (must currently be an ESP32-C3 for the existing `platformio.ini` board type to
  apply as-is).
- LED driver: `bus` (an external driver chip like BP5758/BP6758 over a 2-wire bus -
  confirmed SDA/SCL pins), `pwm_direct` (native ESP32 PWM straight to LED GPIOs -
  confirmed pins), or `none` (no teardown yet / pins unknown).
- If pins/wiring aren't confirmed on real hardware yet, don't invent them - scaffold
  with `driver: "none"` and honest `TODO`s, the same way `RAZER/board.json` does. This
  project's config system exists specifically to keep unverified facts out of the code;
  a guessed pin number is worse than a `TODO`.

## Steps

1. Create `<NAME>/board.json` at the repo root (a sibling of `firmware/`). Start from
   the closest existing example:
   - `OREIN/board.json` for a `bus`-driver board.
   - `VONT/board.json` for a `pwm_direct`-driver board.
   - `RAZER/board.json` for an unconfirmed/no-teardown-yet board.
   Fill in only confirmed fields; leave everything else an honest `TODO`. Full field
   reference: `firmware/README.md`'s "Config reference" section.
2. Add a matching environment to `firmware/platformio.ini`:
   ```ini
   [env:<name>]
   board = lolin_c3_mini
   ```
   (lower-case `<name>`, matching the folder name lower-cased - this is what
   `pio run -e <name>` and the config generator both key off).
3. Add `<name>` to the `board:` matrix in **both**
   `.github/workflows/firmware-build.yml` and `.github/workflows/firmware-release.yml`.
4. If any teardown/pinout notes exist, add them as `<NAME>/readme.md` (see
   `OREIN/readme.md` for the expected shape/depth) and photos under `<NAME>/images/`.
5. Build-verify: `cd firmware && pio run -e <name>`. Fix anything that doesn't compile
   before considering this done - an unbuilt board is worse than no board.
6. Don't touch any other board's files, and don't modify shared `firmware/src`/
   `firmware/include` code unless the new board's LED hardware genuinely doesn't fit
   the existing `pwm_direct`/`bus`/`none` strategies - if so, stop and read
   firmware/README.md's "LED driver abstraction" section first, since that's a bigger
   change than this skill covers.

## Done when

- `pio run -e <name>` succeeds.
- The board appears in both GitHub Actions workflow matrices.
- `<NAME>/board.json` has no invented pin numbers - only confirmed values or explicit
  `TODO`s.
