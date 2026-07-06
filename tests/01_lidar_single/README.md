# Test 01 — Single LiDAR Bring-Up + XSHUT Check

Verifies **one** bare VL53L0X module, and separately proves its XSHUT
pin works, before test 02 chains five of them onto one bus. This is the
first hardware test to run.

## Wiring

Direct to the XIAO — no MCP23017, no mux.

| VL53L0X pin | Connects to |
|---|---|
| VIN (VCC) | XIAO **3.3V** — not 5V, the ESP32-S3 isn't 5V-tolerant |
| GND | XIAO GND |
| SDA | XIAO D4 (GPIO5) |
| SCL | XIAO D5 (GPIO6) |
| XSHUT | XIAO D11 (GPIO42) |
| GPIO1 (interrupt, if present) | not used |

Add 4.7 kΩ pull-ups from SDA/SCL to 3.3V if your breakout doesn't
already include them.

> **Why XSHUT wired directly here, but through the MCP23017 in test 02?**
> One sensor only needs one control line — no reason to involve the
> expander yet. Five sensors need five XSHUT lines, and the XIAO doesn't
> have five spare GPIOs, so test 02 moves XSHUT control onto the
> MCP23017's spare port-B pins instead.

## Run it

```sh
pio run -e test_lidar_single -t upload -t monitor
```

## Expected output

```
[TEST] single VL53L0X — direct wire, no mux, addr 0x29
[TEST] XSHUT check: shutting sensor down, expecting it to disappear from the bus...
[OK] sensor correctly silent while XSHUT is LOW.
[TEST] re-enabling XSHUT, expecting the sensor to come back...
[OK] sensor re-initialised after XSHUT toggle — this pin behaves as test 02 needs it to.
[OK] Printing distance every 100ms.
  345 mm  ==================
  198 mm  =========
```

Wave your hand toward the sensor — the number should track distance in
real time (30–1200 mm range).

## If it fails

- **`sensor still responded with XSHUT held LOW`** — the XSHUT wire isn't
  actually reaching the sensor (floating, cold joint, or wired to the
  wrong pin). This is the important failure to catch *here* — if it's
  wrong on one sensor, it'll be wrong on all five in test 02, and much
  harder to debug once they're chained together.
- **`sensor not found after re-enabling XSHUT`** — check VIN is 3.3V,
  check SDA/SCL aren't swapped, check pull-ups.
- **Reads stuck at 0 or 2000+** — target too close (<30mm) or too far /
  too dark to return a signal; try a light-colored surface at ~200mm.

## Next step

Once both the XSHUT check and the live readings pass, move to
[`tests/02_lidar_array`](../02_lidar_array/README.md) to bring up all
five sensors on one bus via XSHUT re-addressing.
