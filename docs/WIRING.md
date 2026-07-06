# Wiring & Pin Map

> **Implementation status:** this doc describes the VL53L0X + XSHUT
> sensor bus (no mux) validated by `tests/01_lidar_single` and
> `tests/02_lidar_array`. The main firmware (`src/sensor_array.cpp`)
> hasn't been migrated from its older PCA9548A mux design yet — see
> `docs/CONNECTIONS.md`'s status note for the same caveat.

## Power tree (single battery — no separate 3.3 V battery!)

```
3S LiPo 11.1 V 1000 mAh 40C  (XT-60, JST-XH balance)
 ├── TB6612FNG VMOT  (motors, direct)
 └── MP1584EN buck → 5.0 V
      └── XIAO ESP32S3 5V pin
           └── onboard LDO → 3.3 V rail
                ├── ESP32-S3 logic
                ├── MCP23017 VCC          ← power from 3.3 V, NOT 5 V
                ├── 5× VL53L0X VIN        ← power from 3.3 V, NOT 5 V
                └── TB6612FNG VCC (logic)
```

> **Why 3.3 V for all I²C devices:** the XIAO ESP32S3 is *not* 5 V
> tolerant. VL53L0X breakout boards powered at 5 V can drive SDA/SCL to
> 5 V and damage the MCU. All boards work fine at 3.3 V (the module's
> onboard LDO still makes its internal 2.8 V).

## XIAO ESP32S3

| Silk | GPIO | Function |
|---|---|---|
| D0 | 1 | PWMA — left motor PWM (LEDC 20 kHz) |
| D1 | 2 | PWMB — right motor PWM |
| D2 | 3 | Left encoder A (PCNT) |
| D3 | 4 | Left encoder B (PCNT) |
| D4 | 5 | I²C SDA (400 kHz, 4.7 kΩ pull-ups to 3.3 V) |
| D5 | 6 | I²C SCL |
| D6 | 43 | Right encoder A (PCNT) |
| D7 | 44 | Right encoder B (PCNT) |
| D8 | 7 | Battery ADC — 30 kΩ (top) / 10 kΩ (bottom) divider from VBAT |
| D9 | 8 | Status LED (or buzzer) |
| D10 | 9 | Start button → GND (internal pull-up) |
| D11/D12 | 42/41 | Spare (IMU interrupt, etc.) |

## MCP23017 @ 0x20

| Pin | Goes to | Function |
|---|---|---|
| GPA0 | TB6612FNG AIN1 | Left motor direction 1 |
| GPA1 | TB6612FNG AIN2 | Left motor direction 2 |
| GPA2 | TB6612FNG BIN1 | Right motor direction 1 |
| GPA3 | TB6612FNG BIN2 | Right motor direction 2 |
| GPA4 | TB6612FNG STBY | Active low |
| GPB0 | VL53L0X XSHUT | TOF-0, −90° left |
| GPB1 | VL53L0X XSHUT | TOF-1, −30° diag-left (15° printed shroud) |
| GPB2 | VL53L0X XSHUT | TOF-2, 0° front (shroud) |
| GPB3 | VL53L0X XSHUT | TOF-3, +30° diag-right (shroud) |
| GPB4 | VL53L0X XSHUT | TOF-4, +90° right |

## VL53L0X addressing — no mux, XSHUT re-addressing instead

All five sensors ship at the same default address `0x29`, and this
design has no PCA9548A mux to isolate them electrically. Instead, each
sensor's XSHUT pin (driven by the MCP23017, above) is used once at boot
to bring sensors up one at a time and move each to a unique address:

1. Drive all 5 XSHUT lines LOW (every sensor shut down, silent on the bus).
2. Release sensor 0's XSHUT → it wakes at `0x29` → call `setAddress(0x30)`.
3. Repeat for sensors 1–3 → `0x31`, `0x32`, `0x33`.
4. Release sensor 4's XSHUT → leave it at `0x29` (nothing else can claim
   it now that the others have moved off).

After this one-time sequence, all five sensors answer independently and
simultaneously on the shared bus for the rest of the session — XSHUT is
never touched again. See `tests/02_lidar_array/main.cpp` for the working
implementation, and `tests/01_lidar_single` for verifying the XSHUT pin
behaves correctly on a single sensor before wiring all five.

**Why 30°, not 45°:** this track is a single 230 mm-wide corridor with corner
turns only — no side junctions. The diagonal sensors' job is to see the
upcoming corner as early as possible, not to peek into an adjacent branch.
The forward lead-distance a diagonal sensor buys before it detects an
opening scales with `1/tan(θ)` from the direction of travel, so a shallower
angle gives meaningfully more warning (30° gives ~1.7× the lead time of
45°) at the same lateral wall clearance. Below ~25° the wall return gets
unreliable (angle of incidence to the wall approaches grazing), so 30° is
the practical floor without more aggressive tuning.

## FOV-limiting shroud (blinder)

The VL53L0X's native field of view is ~25° — wide enough that, at 30°
mounting spacing, the front (0°) and diagonal beams would otherwise
overlap. A printed aperture tube in front of each of the −30°/0°/+30°
sensors narrows the FOV and stops the cross-talk. The ±90° side sensors
don't need one — nothing else shares their beam path.

Tube length for a target full-angle FOV:

```
L = D / (2 · tan(FOV/2))
```

`D` = the sensor's combined emitter+receiver window diameter (measure it —
don't assume). For a 15° FOV target:

| D | L |
|---|---|
| 4.0 mm | 15.2 mm |
| 4.4 mm | 16.7 mm |
| 5.0 mm | 19.0 mm |

Even at 30° mounting spacing, a 15° shroud leaves ~15° of clear angular
margin before adjacent beams touch — no need to go narrower than the
original spec.

Build notes:
- Print the whole front bumper as one piece with all five bores at their
  mounted angles — guarantees the angles match and is far easier to align
  than five loose tubes.
- Matte-black filament alone isn't enough — flat-black spray paint or
  flocking the bore kills specular IR bounce that plain FDM walls still
  produce.
- Ream the bore with a drill bit sized to `D` after printing — layer lines
  inside a small bore scatter light and quietly widen the effective FOV.
- Seal the tube base against the sensor module with a thin foam gasket so
  stray light can't sneak in around the sides.
- Bench-test before final assembly: sweep a target across the calculated
  cutoff angle and confirm the reading snaps from real-distance to
  no-target right at the edge; lengthen the tube if it doesn't.

## EMI countermeasures (do these BEFORE debugging weird bugs)

- 0.1 µF ceramic **directly across each motor's terminals**
- Ferrite bead on each motor lead, close to the motor
- 100 µF electrolytic + 0.1 µF ceramic across VMOT at the TB6612FNG
- 0.1 µF decoupling at VCC of MCP23017 and each VL53L0X board
- Twisted-pair encoder wiring, routed away from motor leads
- 4.7 kΩ I²C pull-ups (drop to 2.2 kΩ if the bus still glitches)
- Star ground: motor return and logic return meet at one point only
