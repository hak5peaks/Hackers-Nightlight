# Hackers-Nightlight unified firmware

One shared codebase (`src/`, `include/`) builds firmware for every supported bulb.
Per-bulb differences (pins, LED driver, quirks) live in that bulb's own
`<NAME>/board.json` at the repo root. Settings shared by every bulb live in this
directory's `app_config.json`.

## Build & flash

```
cd firmware
pio run -e orein            # or vont, wyze, razer
pio run -e orein -t upload --upload-port /dev/ttyACM1
```

## Adding a new bulb

1. Create `<NEWNAME>/board.json` (copy `OREIN/board.json` as a template) with real,
   hardware-confirmed values - don't guess pins.
2. Add `[env:<newname>]` to `platformio.ini` (usually just `board = lolin_c3_mini`).
3. Add `<newname>` to the `board:` matrix in both `.github/workflows/firmware-*.yml`.
4. `pio run -e <newname>`.

No firmware source changes needed for a normal bulb. If your board's LED hardware
doesn't fit the existing driver strategies, see "LED driver abstraction" below. The
`new-bulb` Claude Code skill (`.claude/skills/new-bulb`) automates steps 1-3.

## Config reference

**`app_config.json`** (shared, overridable per-board):

| Field | Meaning |
|---|---|
| `ap_ssid` / `ap_password` | Default AP credentials once flashed. |
| `ap_hidden` | `true` to stop broadcasting the SSID. |
| `ota_firmware_url` | URL the web UI's `/update` endpoint fetches a `.bin` from. Blank by default - the original project's EC2 host no longer serves firmware, so there's no working generic default. Point this at your own hosting (or override per-board in that board's `board.json`) before relying on OTA; the endpoint refuses to run while this is blank. |

**`<NAME>/board.json`** (one per bulb):

| Field | Meaning |
|---|---|
| `name` / `display_name` | Identifier + human-readable name. |
| `mcu` / `pio_board` | Chip + PlatformIO board target. |
| `led.driver` | `pwm_direct` (native ESP32 PWM, no driver chip - VONT), `bus` (BP5758/BP6758-family chip over a 2-wire bus - WYZE/OREIN), or `none` (RAZER - no LED support yet). |
| `led.led_control_enabled` | Whether the web UI's color picker is live. `false` = wired but not verified on real hardware yet. |
| `led.white_label` / `white_notice` | Text near the white slider. |
| `led.disabled_notice` | Shown instead of controls when disabled. |
| `led.pwm_direct.*` | GPIOs + white-channel count, for that driver. |
| `led.bus.{sda,scl}_pin` | GPIOs, for the bus driver. |
| `led.bus.channel_order` | 3-letter R/G/B permutation for physical wiring vs. the bus protocol's channel order (default `RGB`; OREIN is `BGR`). |
| `led.bus.white_channels` / `boot_white_brightness` | Bus driver details. |
| `led.animations.enabled` | Turns on the web UI's rainbow/pulse/flicker buttons. Bus driver only - forced off for other drivers regardless of this flag (see "Animations" below). |

## OTA firmware update

`POST /update` with `{"ssid": "...", "password": "..."}`. Those credentials are for a
**second, separate WiFi network** - not the device's own `Nightlight` AP. The bulb runs
both an access point and a station radio at once (`WIFI_MODE_APSTA`): the AP is what you
connect *to* for the web UI (`BOARD_AP_SSID`, baked in from config); the STA side is what
this endpoint tells the device to *join*, purely so it has internet access to reach
`ota_firmware_url`. These `ssid`/`password` values live only in the request body - they
are never read from config and never persisted, so they must be supplied on every call.

The device does **not** reboot into the new firmware automatically after a successful
update - only the new image is written and marked bootable
(`esp_ota_set_boot_partition`); the old firmware keeps running until the device resets
for some other reason. An unprompted reboot would drop the AP and
go briefly dark, a visible tell for a device meant to pass as an ordinary lightbulb.

## How it works

`platformio.ini`'s `extra_scripts` runs `boards/generate_board_config.py` before every
build. It reads `../<NAME>/board.json` + `app_config.json`, writes
`include/generated/board_config.h` (plain `#define`s the rest of the code branches on
with `#if`), then calls `test/converter.py` to render `test/input_html.html` into the
compiled-in `page_index.h`. Both generated files reflect whichever board was built most
recently - don't hand-edit them, and rebuild before trusting their contents.

`python3 test/converter.py <board>` previews a board's rendered HTML without a full build.

## LED driver abstraction

Two strategies exist (`pwm_direct`, `bus`) because that's what the current boards use.
A genuinely different strategy needs: a new `LED_DRIVER_*` macro in
`generate_board_config.py`, a matching branch in `system_manager.cpp`'s
`SetupSystem()`/`setRgb()`, and a branch in `webserver.cpp`'s `uri_gpio_led_handler`.

## Animations

Opt-in per board (`led.animations.enabled`), bus driver only. `system_manager.cpp`'s
`tickAnimation()` runs from the main loop and computes rainbow/pulse/flicker
frames from `millis()`; the web UI's buttons hit `/setanimation` with `{"mode": ...}`.
Manually setting a color always cancels whatever animation is running. Currently on
for OREIN only - add `led.animations.enabled: true` to another bus-driver board's
`board.json` to turn it on there too.

## Status

- **VONT/WYZE haven't been build-verified against real hardware** through this unified
  build yet - only OREIN has. Verification so far is code review, not a hardware test.
- **VONT now shares OREIN's newer web UI** (color swatches, single-click select) as
  part of using one template for every board. Compatible with VONT's existing
  `/setcolor` contract; if a separate, simpler UI is wanted for VONT instead, a
  per-board template is a small addition (the token-substitution mechanism supports it).
- The `/update` OTA endpoint uses plaintext HTTP with no signature check - unchanged
  from before, now a single place to fix instead of several.
