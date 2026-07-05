# Test 01 — Single LiDAR (VL53L0X) Bring-Up

Verifies **one** bare TOF200C module before you add the PCA9548A mux or
the other four sensors. This is the first hardware test to run.

## Wiring

No mux needed yet — wire the sensor directly to the XIAO.

| TOF200C pin | Connects to |
|---|---|
| VIN | XIAO **3.3V** — not 5V, the ESP32-S3 isn't 5V-tolerant |
| GND | XIAO GND |
| SDA | XIAO D4 (GPIO5) |
| SCL | XIAO D5 (GPIO6) |

Add 4.7 kΩ pull-up resistors from SDA and SCL to 3.3V if your breakout
doesn't already have them onboard (most TOF200C boards do — check first).

## Run it

```sh
pio run -e test_lidar_single -t upload -t monitor
```

## Expected output

```
[TEST] single VL53L0X — direct wire, no mux, addr 0x29
[OK] sensor initialised. Printing distance every 100ms.
  345 mm  ==================
  198 mm  =========
  812 mm  ========================================
```

Wave your hand toward the sensor — the number should track distance in
real time (30–1200 mm range), and the bar graph should grow/shrink with
it.

## If it fails

- **`[FAIL] sensor not found`** — check VIN is 3.3V (not 5V), check SDA/SCL
  aren't swapped, check the breakout's onboard address jumper isn't set
  away from default.
- **Reads stuck at 0 or 2000+** — target too close (<30mm) or too far /
  too dark to return a signal; try a light-colored surface at ~200mm.
- **I2C bus scan shows nothing** — verify pull-ups; some XIAO clones need
  external 4.7 kΩ if the module doesn't include them.

## Next step

Once this passes, move to
[`tests/02_lidar_array`](../02_lidar_array/README.md) to bring up all
five sensors through the mux.
