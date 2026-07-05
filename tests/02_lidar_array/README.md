# Test 02 — Full 5× LiDAR Array (PCA9548A Mux)

Brings up all five VL53L0X sensors behind the I²C mux. Run
[`tests/01_lidar_single`](../01_lidar_single/README.md) first on at least
one loose sensor so you know a bad reading here means a **mux/mounting**
problem, not a bad sensor.

## Wiring

| Component | Connects to |
|---|---|
| PCA9548A VCC | XIAO 3.3V |
| PCA9548A GND | XIAO GND |
| PCA9548A SDA/SCL | XIAO D4/D5 (main bus) |
| PCA9548A CH0 → TOF-0 | −90° left wall sensor |
| PCA9548A CH1 → TOF-1 | −30° diagonal-left sensor |
| PCA9548A CH2 → TOF-2 | 0° front sensor |
| PCA9548A CH3 → TOF-3 | +30° diagonal-right sensor |
| PCA9548A CH4 → TOF-4 | +90° right wall sensor |

Every sensor's VIN → 3.3V (not 5V), every sensor's SDA/SCL → the
corresponding mux channel's SD/SC pins (**not** the mux's upstream
SDA/SCL — those go to the XIAO).

## Run it

```sh
pio run -e test_lidar_array -t upload -t monitor
```

## Expected output

Init report (once, at boot):
```
[TEST] 5x VL53L0X via PCA9548A mux (addr 0x70)
  CH0 TOF-0 -90 LEFT  : OK
  CH1 TOF-1 -30 DIAG-L : OK
  CH2 TOF-2   0 FRONT  : OK
  CH3 TOF-3 +30 DIAG-R : OK
  CH4 TOF-4 +90 RIGHT  : OK
```

Then a continuous stream:
```
L: 210  DL: 340  F: 890  DR: 355  R: 205
```

**Sanity check:** sweep a hand slowly left-to-right in front of the
bumper. The dip in reading should travel across the columns in order —
L → DL → F → DR → R — matching the physical mounting order. If it jumps
around out of order, two sensors are wired to the wrong mux channels.

## If it fails

- **One channel `FAIL`** — that sensor or its mux channel wiring is bad;
  isolate it with `tests/01_lidar_single` (swap it onto the direct-wire
  test setup).
- **All channels `FAIL`** — the mux itself isn't responding; check its
  VCC/GND/SDA/SCL to the XIAO, and confirm nothing else on the bus is
  address-conflicting at 0x70.
- **Readings cross-talk** (a sensor sees a "wall" that isn't there,
  especially between adjacent angles) — this is exactly what the FOV
  shroud in `docs/REPORT.md` §2.4 exists to fix. Confirm it's installed
  and sized correctly before blaming the sensor.

## Next step

Sensors verified — move on to
[`tests/03_motor_encoder`](../03_motor_encoder/README.md) for the drive
system, or [`tests/04_oled_display`](../04_oled_display/README.md) if
you're bringing up the optional status display.
