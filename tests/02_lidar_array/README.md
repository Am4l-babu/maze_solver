# Test 02 — Full 5x LiDAR Array via XSHUT Re-Addressing

Brings up all five VL53L0X sensors on **one shared I2C bus, no mux** —
each sensor's XSHUT pin (driven by the MCP23017) lets them power up one
at a time during boot and claim a unique address, then all five answer
simultaneously from then on. Run
[`tests/01_lidar_single`](../01_lidar_single/README.md) first so a
failure here means a wiring/mounting problem, not a bad sensor or an
untested XSHUT pin.

## Wiring

| Component | Connects to |
|---|---|
| MCP23017 VCC/GND | XIAO 3.3V / GND |
| MCP23017 SDA/SCL | XIAO D4/D5 (main bus, addr 0x20) |
| MCP23017 GPB0 → TOF-0 XSHUT | −90° left wall sensor |
| MCP23017 GPB1 → TOF-1 XSHUT | −30° diagonal-left sensor |
| MCP23017 GPB2 → TOF-2 XSHUT | 0° front sensor |
| MCP23017 GPB3 → TOF-3 XSHUT | +30° diagonal-right sensor |
| MCP23017 GPB4 → TOF-4 XSHUT | +90° right wall sensor |
| All 5× VL53L0X VIN/GND | XIAO 3.3V / GND (shared) |
| All 5× VL53L0X SDA/SCL | XIAO D4/D5 (shared main bus) |

Every sensor's SDA/SCL goes to the **same** two bus wires — there's no
per-channel wiring like the mux version had. Only XSHUT is unique per
sensor.

> MCP23017 port A (pins 0-4) stays reserved for motor direction/STBY on
> the real robot, even though this standalone test never drives a motor
> — keeping port B for XSHUT here matches the final wiring exactly.

## What happens at boot

1. All 5 XSHUT lines are driven **LOW** together — every sensor is
   powered down and silent, so none of them can collide at the shared
   default address `0x29`.
2. One at a time: XSHUT → HIGH, sensor wakes at `0x29`, firmware
   immediately calls `setAddress()` to move it to a unique address
   (`0x30`, `0x31`, `0x32`, `0x33`), then moves to the next sensor.
3. The fifth sensor keeps the default `0x29` — nothing else is left to
   claim it.
4. All five now answer independently, at all times, on the one shared
   bus. XSHUT is never touched again after boot.

## Run it

```sh
pio run -e test_lidar_array -t upload -t monitor
```

## Expected output

```
[TEST] 5x VL53L0X via XSHUT re-addressing (no mux)
  TOF-0 -90 LEFT  -> 0x30 : OK
  TOF-1 -30 DIAG-L -> 0x31 : OK
  TOF-2   0 FRONT  -> 0x32 : OK
  TOF-3 +30 DIAG-R -> 0x33 : OK
  TOF-4 +90 RIGHT  -> 0x29 : OK
L: 210  DL: 340  F: 890  DR: 355  R: 205
```

**Sanity check:** sweep a hand slowly left-to-right in front of the
bumper. The dip in reading should travel across the columns in order —
L → DL → F → DR → R — matching the physical mounting order. If it jumps
around out of order, two sensors' XSHUT wires are swapped on the
MCP23017.

## If it fails

- **One sensor `FAIL`** — its XSHUT never reached the MCP23017 pin
  correctly (bad joint, wrong GPB pin), or the sensor itself is bad.
  Isolate it with `tests/01_lidar_single`'s wiring (swap it onto the
  direct-wire rig).
- **All sensors `FAIL`** — MCP23017 isn't responding; check its own
  wiring/address (`0x20`) before blaming the sensors.
- **A sensor `OK`s but the readings jump around out of physical order**
  — two XSHUT lines are swapped between GPB pins; the *address* each
  sensor got is fine, but it's mounted where you think a different one
  is.
- **Cross-talk between adjacent readings** — this is what the FOV
  shroud in `docs/REPORT.md` §2.4 exists to fix; confirm it's installed
  before suspecting the addressing scheme.

## Next step

Sensors verified — move on to
[`tests/03_motor_encoder`](../03_motor_encoder/README.md) for the drive
system, or [`tests/04_oled_display`](../04_oled_display/README.md) if
you're bringing up the optional setup/status display.
