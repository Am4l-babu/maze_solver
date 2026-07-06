# Master Connections Reference

One table per bus/subsystem. Everything the robot needs to move, sense, and
report status — including the SSD1306 OLED (new, test-only for now, not yet
wired into the main firmware loop). For narrative wiring rationale and EMI
countermeasures, see [WIRING.md](WIRING.md). For the reasoning behind the
±30° diagonal angle and the FOV shroud, see [REPORT.md](REPORT.md).

> **Implementation status:** the sensor bus architecture below (XSHUT
> re-addressing, no mux) is the design validated by
> [`tests/01_lidar_single`](../tests/01_lidar_single) and
> [`tests/02_lidar_array`](../tests/02_lidar_array). The main robot
> firmware (`src/sensor_array.cpp`) has **not been migrated yet** — it
> still implements the older PCA9548A mux design. Wire and test the
> array using this doc; port `src/` to match once it's proven on real
> hardware.

---

## 1. XIAO ESP32S3 — every pin, in one place

| Silk | GPIO | Wire goes to | Signal | Notes |
|---|---|---|---|---|
| 3V3 | — | MCP23017 VCC, 5× VL53L0X VIN, SSD1306 VCC | 3.3 V rail | **Never wire any of these to 5V** — ESP32-S3 I²C pins are not 5V-tolerant |
| GND | — | common ground (star topology) | GND | Motor return joins here only, not mid-bus |
| 5V  | — | MP1584EN buck output | 5 V in | Powers the XIAO's onboard 3.3V LDO only |
| D0  | GPIO1  | TB6612FNG PWMA | PWM out | Hardware LEDC, 20 kHz |
| D1  | GPIO2  | TB6612FNG PWMB | PWM out | Hardware LEDC, 20 kHz |
| D2  | GPIO3  | Left encoder channel A | digital in | PCNT hardware decode |
| D3  | GPIO4  | Left encoder channel B | digital in | PCNT hardware decode |
| D4  | GPIO5  | I²C SDA — MCP23017, 5× VL53L0X, SSD1306 | I²C | 400 kHz, 4.7 kΩ pull-ups to 3.3 V |
| D5  | GPIO6  | I²C SCL — same bus | I²C | shared with everything above |
| D6  | GPIO43 | Right encoder channel A | digital in | PCNT hardware decode |
| D7  | GPIO44 | Right encoder channel B | digital in | PCNT hardware decode |
| D8  | GPIO7  | Battery divider midpoint (30kΩ/10kΩ) | analog in | `ADC_11db` attenuation |
| D9  | GPIO8  | Status LED (or buzzer) | digital out | Fast-GPIO in main firmware |
| D10 | GPIO9  | Start button → GND | digital in | Internal pull-up, active low |
| D11 | GPIO42 | *spare* | — | Candidate: IMU interrupt |
| D12 | GPIO41 | *spare* | — | Candidate: IMU interrupt |

No PCA9548A mux in this design — dropped in favor of VL53L0X's XSHUT pin
(see §2a). One less component on the bus, one less thing to wire.

---

## 2. I²C bus — every device, one bus, six addresses

All devices share **SDA→D4, SCL→D5**, 3.3 V logic, 4.7 kΩ pull-ups. No
mux — every device, including all 5 sensors, answers directly on this one
shared bus at its own unique address.

| Address | Device | Purpose |
|---|---|---|
| `0x20` | MCP23017 | Motor direction (AIN1/2, BIN1/2) + TB6612 STBY + sensor XSHUT lines |
| `0x29` | VL53L0X (TOF-4, +90° right) | Kept at power-on default — last sensor brought up, nothing left to collide with it |
| `0x30` | VL53L0X (TOF-0, −90° left) | Reassigned at boot via `setAddress()` |
| `0x31` | VL53L0X (TOF-1, −30° diag-left) | Reassigned at boot |
| `0x32` | VL53L0X (TOF-2, 0° front) | Reassigned at boot |
| `0x33` | VL53L0X (TOF-3, +30° diag-right) | Reassigned at boot |
| `0x3C` (or `0x3D`) | SSD1306 OLED | 128×64 setup/runtime display — **test-only**, not yet in main firmware |

## 2a. VL53L0X addressing sequence (XSHUT, via MCP23017)

All five sensors ship with the same default address (`0x29`) and no mux
isolates them anymore, so each one is moved to a unique address once at
boot, in order:

1. Drive **all 5** MCP23017-controlled XSHUT lines LOW — every sensor
   powered down and silent, so none can collide on `0x29` yet.
2. For sensors 0–3: XSHUT → HIGH, sensor wakes at `0x29`, immediately
   call `setAddress()` to move it off `0x29`, then move to the next.
3. Sensor 4: XSHUT → HIGH, leave it at `0x29` — nothing else will claim
   that address afterward.
4. All five now answer independently, simultaneously, forever (until
   power-off) — XSHUT is never touched again after boot.

| MCP23017 pin | Controls | Sensor | Angle | Role |
|---|---|---|---|---|
| GPB0 | XSHUT | TOF-0 | −90° | Left wall / centering |
| GPB1 | XSHUT | TOF-1 | −30° | Diagonal-left / corner lead |
| GPB2 | XSHUT | TOF-2 | 0° | Front / brake trigger |
| GPB3 | XSHUT | TOF-3 | +30° | Diagonal-right / corner lead |
| GPB4 | XSHUT | TOF-4 | +90° | Right wall / centering |
| GPB5–7 | *spare* | — | — | Future expansion |

Validated in [`tests/02_lidar_array`](../tests/02_lidar_array) — see that
test's `main.cpp` for the exact boot sequence in code.

---

## 3. MCP23017 — motor direction (port A) + sensor XSHUT (port B)

| Pin | Goes to | Function |
|---|---|---|
| GPA0 | TB6612FNG AIN1 | Left motor direction 1 |
| GPA1 | TB6612FNG AIN2 | Left motor direction 2 |
| GPA2 | TB6612FNG BIN1 | Right motor direction 1 |
| GPA3 | TB6612FNG BIN2 | Right motor direction 2 |
| GPA4 | TB6612FNG STBY | Standby, active low (LOW = disabled) |
| GPA5–7 | *spare* | Future expansion |
| GPB0–4 | 5× VL53L0X XSHUT | Sensor addressing at boot — see §2a |
| GPB5–7 | *spare* | Future expansion |

---

## 4. TB6612FNG motor driver

| Pin | Connects to | Notes |
|---|---|---|
| VMOT | 3S LiPo, direct | Up to 13.5 V rated; pack is ~11.1–12.6 V |
| VCC | 3.3 V rail | Logic supply — **not** 5 V |
| PWMA / PWMB | XIAO D0 / D1 | 20 kHz hardware PWM |
| AIN1/AIN2, BIN1/BIN2 | MCP23017 GPA0–3 | Direction — see §3 |
| STBY | MCP23017 GPA4 | Active low |
| AO1/AO2 | Left motor terminals | 0.1µF ceramic cap across these terminals |
| BO1/BO2 | Right motor terminals | 0.1µF ceramic cap across these terminals |
| GND | Common star ground | Motor return joins main ground at one point only |

---

## 5. Encoders (quadrature, on-motor)

| Motor | Channel A | Channel B | XIAO pins |
|---|---|---|---|
| Left | white/signal 1 | white/signal 2 | D2 (A), D3 (B) |
| Right | white/signal 1 | white/signal 2 | D6 (A), D7 (B) — **wired A/B swapped** in firmware to correct for the right motor's mirrored mounting |

Encoder logic supply: 3.3 V from the same rail as everything else (most
N20 magnetic encoders are 3.3–5 V tolerant on the signal lines — check
your specific module).

---

## 6. Battery & power tree

```
3S LiPo 11.1V 1000mAh 40C (XT-60, JST-XH balance)
 ├── TB6612FNG VMOT (motors, direct)
 ├── 30kΩ/10kΩ divider → XIAO D8 (battery voltage sense)
 └── MP1584EN buck → 5.0V → XIAO 5V pin → onboard LDO → 3.3V rail
                                                            ├── MCP23017
                                                            ├── 5× VL53L0X
                                                            ├── SSD1306 (if wired)
                                                            └── TB6612FNG logic (VCC)
```

---

## 7. New: SSD1306 0.96" OLED (128×64, I²C, blue)

| Pin | Connects to |
|---|---|
| VCC | 3.3 V rail |
| GND | Common ground |
| SDA | XIAO D4 (shared main bus) |
| SCL | XIAO D5 (shared main bus) |

- Default I²C address `0x3C`; some clone boards ship at `0x3D` — the test
  sketch (`tests/04_oled_display`) tries both automatically.
- Rated 20 mA @ 3.3 V — negligible load on the existing buck/LDO chain, no
  power-tree changes needed.
- **Role: setup/calibration screens + a persisted total-runtime
  hour-meter — never a live in-run display.** It's only ever updated
  during `IDLE`/`CALIBRATING`/`FINISHED`, at a slow (~1 Hz) rate, so it
  never competes with the TOF sensor sweep for I²C bus time during a
  timed run. See `tests/04_oled_display` for the working demo (splash
  screen, test pattern, then the NVS-persisted runtime counter).
- **Not yet wired into the main robot firmware** — currently a standalone
  test only. Porting it in means adding it to `env:xiao_esp32s3`'s
  `lib_deps` and calling into it only from the non-`RUNNING` states in
  `src/main.cpp`.

---

## Wiring checklist before first power-up

- [ ] Every I²C device's VCC goes to **3.3 V**, none to 5 V
- [ ] 4.7 kΩ pull-ups present on SDA and SCL (once, not per-device)
- [ ] Common star ground: motor return and logic return meet at one point
- [ ] 0.1 µF ceramic caps across both motor terminals, ferrite bead on each motor lead
- [ ] Battery divider verified against a multimeter before trusting `readBatteryVoltage()`
- [ ] Each VL53L0X's blinder/shroud seated flush against its optical window (see REPORT.md §2.4)
- [ ] Each VL53L0X's XSHUT wired to its correct MCP23017 GPB pin — verify with `tests/01_lidar_single`'s XSHUT check before wiring all five
