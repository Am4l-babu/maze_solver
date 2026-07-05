# Test 04 — SSD1306 OLED Display (128×64, I²C)

Brings up the 0.96" blue SSD1306 OLED. This is a **new component**, not
yet wired into the main robot firmware — see "Suggestions & Upgrades" in
[`tests/README.md`](../README.md) for how it could feed live status once
proven out here.

## Module spec

| Spec | Value |
|---|---|
| Resolution | 128×64 pixels, monochrome |
| Interface | I²C |
| Power | 3.3V, ~20 mA |
| Driver | SSD1306 |

## Wiring

| OLED pin | Connects to |
|---|---|
| VCC | XIAO 3.3V |
| GND | XIAO GND |
| SDA | XIAO D4 (GPIO5) — shared main bus |
| SCL | XIAO D5 (GPIO6) — shared main bus |

At 20 mA this rides on the existing 3.3V rail with no power-tree changes.
It shares the bus with the MCP23017 and PCA9548A — no address conflict
(OLED is 0x3C/0x3D, others are 0x20/0x70).

## Run it

```sh
pio run -e test_oled -t upload -t monitor
```

## Expected behavior

1. **Splash screen** (~1.5s): "MICRO-MAZE SOLVER / PRIM.E Chapter II" then
   large "OLED OK" text.
2. **Test pattern** (~2s): a full-frame border rectangle with a filled
   dot in each of the 4 corners, plus "PATTERN" centered. This is the
   important one — if the border looks shifted, clipped, or mirrored,
   your panel's column/row offset differs from the driver default (some
   clone SSD1306 boards need `Wire.setClock()` tweaks or a different
   `Adafruit_SSD1306` constructor offset).
3. **Live counter** (continuous): frame count, uptime, and a bar that
   slides left-to-right and wraps. If this freezes, the I²C bus or the
   display driver has hung.

## If it fails

- **`[FAIL] no SSD1306 found at 0x3C or 0x3D`** — check VCC is 3.3V (most
  of these boards do NOT like 5V despite some vendors claiming 3–5V
  tolerance), check SDA/SCL aren't swapped.
- **Garbled/scrambled pixels** — usually a power brown-out during init;
  add a 0.1µF ceramic across the OLED's VCC/GND right at the module.
- **Test pattern border is cut off on one edge** — some cheap clones are
  wired for a 132×64 SSD1306 controller instance displaying onto a
  128×64 panel, causing a 2-4px horizontal offset. If this happens,
  it's a known clone-board quirk, not your wiring.

## Suggestion: on-robot status display

If you decide to wire this onto the real robot (not required, purely
optional), natural things to show during a run:
- Current state (IDLE/CALIBRATING/RUNNING/FINISHED/FAULT)
- Battery voltage, updated live
- A 5-bar mini bar-graph of the live sensor array (great for spotting a
  sensor dropout mid-run without a laptop attached)

This would need `Adafruit_SSD1306`/`Adafruit_GFX` added to the main
`env:xiao_esp32s3` lib_deps, and — since `display.display()` pushes the
full 1KB framebuffer over I²C — should be rate-limited (e.g. update once
every 100-200ms, not every 100Hz tick) so it doesn't compete with the
TOF sensor sweep for bus time.
