# Wiring & Pin Map

## Power tree (single battery — no separate 3.3 V battery!)

```
3S LiPo 11.1 V 1000 mAh 40C  (XT-60, JST-XH balance)
 ├── TB6612FNG VMOT  (motors, direct)
 └── MP1584EN buck → 5.0 V
      └── XIAO ESP32S3 5V pin
           └── onboard LDO → 3.3 V rail
                ├── ESP32-S3 logic
                ├── MCP23017 VCC          ← power from 3.3 V, NOT 5 V
                ├── PCA9548A VCC          ← power from 3.3 V, NOT 5 V
                ├── 5× TOF200C VIN        ← power from 3.3 V, NOT 5 V
                └── TB6612FNG VCC (logic)
```

> **Why 3.3 V for all I²C devices:** the XIAO ESP32S3 is *not* 5 V
> tolerant. TOF200C boards powered at 5 V can drive SDA/SCL to 5 V and
> damage the MCU. All boards work fine at 3.3 V (the TOF200C's onboard
> LDO still makes its internal 2.8 V).

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

## MCP23017 @ 0x20 (port A)

| Pin | TB6612FNG |
|---|---|
| GPA0 | AIN1 (left dir 1) |
| GPA1 | AIN2 (left dir 2) |
| GPA2 | BIN1 (right dir 1) |
| GPA3 | BIN2 (right dir 2) |
| GPA4 | STBY (active low) |

## PCA9548A @ 0x70

| Channel | Sensor | Angle |
|---|---|---|
| CH0 | TOF-0 | −90° left |
| CH1 | TOF-1 | −30° diag-left (15° printed shroud) |
| CH2 | TOF-2 | 0° front (shroud) |
| CH3 | TOF-3 | +30° diag-right (shroud) |
| CH4 | TOF-4 | +90° right |

All five sensors keep default address 0x29 — the mux isolates them.

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
- 0.1 µF decoupling at VCC of MCP23017, PCA9548A, and each TOF board
- Twisted-pair encoder wiring, routed away from motor leads
- 4.7 kΩ I²C pull-ups (drop to 2.2 kΩ if the bus still glitches)
- Star ground: motor return and logic return meet at one point only
