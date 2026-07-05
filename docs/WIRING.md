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
| CH1 | TOF-1 | −45° diag-left (15° printed blinder) |
| CH2 | TOF-2 | 0° front (blinder) |
| CH3 | TOF-3 | +45° diag-right (blinder) |
| CH4 | TOF-4 | +90° right |

All five sensors keep default address 0x29 — the mux isolates them.

## EMI countermeasures (do these BEFORE debugging weird bugs)

- 0.1 µF ceramic **directly across each motor's terminals**
- Ferrite bead on each motor lead, close to the motor
- 100 µF electrolytic + 0.1 µF ceramic across VMOT at the TB6612FNG
- 0.1 µF decoupling at VCC of MCP23017, PCA9548A, and each TOF board
- Twisted-pair encoder wiring, routed away from motor leads
- 4.7 kΩ I²C pull-ups (drop to 2.2 kΩ if the bus still glitches)
- Star ground: motor return and logic return meet at one point only
