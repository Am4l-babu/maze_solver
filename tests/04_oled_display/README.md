# Test 04 — SSD1306 OLED (128×64, I²C): Setup Screens + Runtime Hour-Meter

Brings up the 0.96" blue SSD1306 OLED. **This display's actual role on
the robot is setup/calibration screens and a persisted total-runtime
counter — it is never updated while the robot is in a timed run.**
That's a deliberate constraint, not a limitation of the hardware: the
100 Hz control loop needs the I2C bus fully available for the TOF
sensor sweep, and an OLED push (~1KB framebuffer over I2C) has no
business competing with that mid-run. This test's demo reflects the
real intended usage — nothing high-frequency happens here.

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

At 20 mA this rides on the existing 3.3V rail with no power-tree
changes. Shares the bus with the MCP23017 and the 5 VL53L0X sensors —
no address conflict (OLED is 0x3C/0x3D; MCP23017 is 0x20; sensors are
0x29/0x30-0x33 after XSHUT re-addressing, see `tests/02_lidar_array`).

## Run it

```sh
pio run -e test_oled -t upload -t monitor
```

## Expected behavior

1. **Splash screen** (~1.5s): "MICRO-MAZE SOLVER / PRIM.E Chapter II"
   then large "OLED OK" text. This is the setup-time role — shown once
   at boot, gone before anything moves.
2. **Test pattern** (~2s): a full-frame border rectangle with a filled
   dot in each of the 4 corners, plus "PATTERN" centered. If the border
   looks shifted, clipped, or mirrored, your panel's column/row offset
   differs from the driver default (some clone SSD1306 boards need a
   different `Adafruit_SSD1306` constructor offset).
3. **Runtime counter** (continuous, but only **once per second** — not a
   live/high-refresh display): shows total lifetime runtime (`XhYYmZZs`)
   and the current session's elapsed time. The total is persisted to
   flash (NVS via `Preferences`) and survives a reset or power cycle —
   reset the board and confirm the total picks up from where it left off
   instead of restarting at zero.

## If it fails

- **`[FAIL] no SSD1306 found at 0x3C or 0x3D`** — check VCC is 3.3V (most
  of these boards do NOT like 5V despite some vendors claiming 3–5V
  tolerance), check SDA/SCL aren't swapped.
- **Garbled/scrambled pixels** — usually a power brown-out during init;
  add a 0.1µF ceramic across the OLED's VCC/GND right at the module.
- **Total runtime resets to 0 every boot** — the NVS write isn't landing
  before reset; confirm `prefs.putUInt()` is actually being reached (add
  a print) and that the board isn't losing power faster than the ~1s
  write cadence during your test resets.

## Why setup-only, and how the counter works

- **Never during a run**: the main firmware's `STATE_RUNNING` never
  touches the display. It's updated during `IDLE`/`CALIBRATING`/
  `FINISHED` only — exactly the states where the I2C bus isn't
  contested by anything time-critical.
- **1 Hz update rate**: the runtime counter changes at most once a
  second by nature, so there's no reason to redraw faster — this also
  keeps the habit of treating the OLED as a slow, occasional device
  rather than something that grows into a live dashboard by accident.
- **Flash wear**: this test writes to NVS once per second to make the
  persistence visible quickly. On the real robot, throttle that to
  roughly once a minute — NVS flash has a finite (though large, ~100k)
  write-cycle budget, and an hour-meter doesn't need second-level
  precision.

## Next step

All five bring-up tests done. Flash the real robot firmware:
`pio run -e xiao_esp32s3 -t upload`. Note that `src/` doesn't wire the
OLED in yet — see `tests/README.md`'s Suggestions section for that as a
follow-on integration step.
