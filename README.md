# 🤖 Micro-Maze Solver

> A blazingly fast, wall-touch-free autonomous maze robot built for the **IEEE RAS SBC GECT PRIM.E Chapter II Maze Solver Competition (July 2026)**.

![Status](https://img.shields.io/badge/status-competition--ready-brightgreen) ![Build](https://img.shields.io/badge/build-passing-brightgreen) ![License](https://img.shields.io/badge/license-MIT-blue)

---

## 🎯 The Challenge

Navigate a **single-path maze** without wall contact. Every touch forfeits that section's points. **Speed wins only if precision comes first.**

- **Track:** 230 mm wide corridor, 8 ft × 8 ft arena
- **Rules:** No dead ends, no junctions — just corners and straightaways
- **Scoring:** Sections cleared (10 pts/section × 8 sections), then time as tiebreaker
- **Time limit:** 12 minutes total (3 runs, best of 3)

**Our strategy:** Wall-touch avoidance trumps raw speed. Centered corridor-following at aggressive speed, v² braking into corners, 90° pivot turns.

---

## 🛠️ Hardware at a Glance

| Subsystem | Part | Notes |
|-----------|------|-------|
| **Brain** | XIAO ESP32S3 | 240 MHz dual-core, hardware quadrature decode |
| **Eyes** | 5× VL53L0X (TOF200C) | ±90°, ±30°, 0° with FOV-limiting shrouds |
| **Drive** | TB6612FNG + 2× N20 gearmotor | 150:1, active short-brake for stopping |
| **Wheels** | 2× ⌀43 mm + front caster | Differential drive, ~95 mm wheelbase |
| **Power** | 3S LiPo 1000 mAh | Single battery → 5V buck → 3.3V LDO → all logic |
| **Dimensions** | 120×125×55 mm | Fits 150×150×100 mm limit, under 100 mm wall height |

**Key insight:** All I²C devices (sensors, mux, expander) powered from 3.3 V — the ESP32-S3 is not 5 V tolerant.

---

## 📐 Sensor Array: Why ±30° Diagonals?

The original design brief mounted diagonals at ±45° (classic micromouse). This competition is different:

- **Single corridor** → no side branches to peek into
- **Corner turns** only → the job is maximum **early-warning lead time**, not junction detection
- **Math:** forward lead-distance ∝ 1/tan(θ) → 30° gives **1.7× more warning** than 45° at the same side clearance

**Result:** ±30° diagonals, each with a 15° FOV-limiting shroud to stop optical cross-talk.

| Sensor | Angle | Role |
|--------|-------|------|
| TOF-0 | −90° | Left wall / centering |
| TOF-1 | −30° | **Diagonal-left** — sees corner approach |
| TOF-2 | 0° | Front / brake trigger |
| TOF-3 | +30° | **Diagonal-right** — sees corner approach |
| TOF-4 | +90° | Right wall / centering |

Read [docs/WIRING.md](#foving-shroud-build-spec) for the shroud design formula, bore-length table, and 3D-print recipe.

---

## 🎮 Control Strategy (100 Hz Loop)

### State machine
```
IDLE
  ↓ (START button)
CALIBRATING (1s settle, zero encoders)
  ↓
RUNNING (corridor centering + corner detection)
  ├→ BRAKE_FOR_TURN (v² profile: v = √(2·a·d))
  ├→ PIVOT (encoder-counted in-place 90° turn)
  ├→ EXIT_TURN (60mm blind straight, resync sensors)
  └→ FOLLOW (back to centering)
  ↓ (corridor fully opens → finish line)
FINISHED (fast blink, re-arm for next round)

FAULT (low battery / sensor loss) → safe stop
```

### Inner loop: per-motor speed PID
- **Input:** encoder mm/s (hardware PCNT, zero CPU cost)
- **Tuning:** derivative-on-measurement with low-pass filtering (encoder quantization at 450 CPR / 100 Hz is ~30 mm/s)
- **Voltage compensation:** scales PWM by `11.1V / V_batt` so tuning holds as battery sags across 3 back-to-back runs

### Outer loop: wall-centering PID
- **Both walls present:** `error = (left_dist − right_dist) / 2`
- **One wall (pre-corner):** hold ≈55 mm offset from the open side
- **Diagonal term:** adds approach-angle correction via diagonal sensor difference
- **Speed governance:** cruise speed scales down by centering error — the bot earns speed by staying centered

---

## 📦 Repository Layout

```
src/
├── main.cpp              ← 100 Hz control loop, state machine
├── config.h              ← ALL tuning knobs + physical constants
├── pid_controller.*      ← Generic PID (anti-windup, filtered D)
├── sensor_array.*        ← 5× VL53L0X via PCA9548A mux, core-0 task
├── motor_driver.*        ← TB6612FNG + MCP23017 direction pins
├── encoder.*             ← Hardware PCNT quadrature → mm/s
├── navigator.*           ← Corridor centering, braking, pivots
├── state_machine.h       ← IDLE/CALIBRATING/RUNNING/FINISHED/FAULT
├── maze_map.*            ← Flood-fill fallback (unused in PRIM.E mode)
├── i2c_bus.h             ← Cross-core I²C mutex (shared bus, 2 cores)
└── fast_gpio.h           ← Direct-register GPIO for the hot loop

tests/                    ← Standalone hardware bring-up sketches
├── README.md             ← Test order, suggestions & upgrades
├── 01_lidar_single/      ← One bare VL53L0X, no mux
├── 02_lidar_array/       ← All 5 sensors + PCA9548A mux
├── 03_motor_encoder/     ← Forward/back/speed, gear-ratio check
├── 04_oled_display/      ← SSD1306 128×64 status display (new)
└── 05_push_button/       ← Start-button debounce & wiring

docs/
├── CONNECTIONS.md        ← Master wiring reference, every pin/wire
├── WIRING.md             ← Power tree, pin map, shroud build spec
├── REPORT.md             ← Design trade-offs, angle decision, competition plan
└── (layout blueprints in artifact form)

platformio.ini            ← PlatformIO: XIAO ESP32S3 + pinned libs + 5 test envs
```

---

## 🧪 Testing & Bring-Up

Before trusting the full robot firmware, prove each subsystem in
isolation. Five standalone PlatformIO environments live in
[`tests/`](tests/README.md) — flash one at a time, in order:

```bash
pio run -e test_lidar_single  -t upload -t monitor   # 1. one bare sensor
pio run -e test_lidar_array   -t upload -t monitor   # 2. all 5 + mux
pio run -e test_motor_encoder -t upload -t monitor   # 3. forward/back/speed
pio run -e test_button        -t upload -t monitor   # 4. start button
pio run -e test_oled          -t upload -t monitor   # optional: new OLED
```

See [`tests/README.md`](tests/README.md) for the full test order,
expected output for each, and a running list of suggested upgrades that
came out of writing them. Full pin-by-pin wiring for the whole robot —
including the OLED — lives in [`docs/CONNECTIONS.md`](docs/CONNECTIONS.md).

---

## ⚡ Build & Flash

### Requirements
- [PlatformIO](https://platformio.org/) (CLI or VS Code)
- XIAO ESP32S3 with USB-C cable

### Build
```bash
pio run                    # compile for XIAO ESP32S3
pio device monitor         # 115200 baud, watch telemetry (BEFORE competition)
```

### Flash & telemetry
```bash
pio run -t upload          # over USB-C
```

### For competition day
```bash
pio run -DCOMPETITION_BUILD  # strips all telemetry (rule 13: no wireless once run starts)
```

---

## ✅ Before First Run — Verify on Hardware

Three unknowns from conflicting vendor datasheets — **all three are one-constant fixes**:

| Item | Check | Why it matters |
|------|-------|----------------|
| **Gear ratio** | Roll wheel exactly 10 turns by hand. Count encoder pulses. Compare to `COUNTS_PER_REV` in `src/config.h`. Vendors list both 100:1 and 150:1. | Off by 50%? Speed PID will either overshoot or stall. |
| **Wheelbase** | Measure wheel-centre to wheel-centre. Update `WHEELBASE_MM` (firmware assumes 95 mm). | Pivot turn accuracy is directly proportional. Even ±5 mm matters. |
| **TOF offsets** | Centre bot in a 230 mm corridor. Both side sensors should read ≈55 mm. Trim per-sensor offsets in `sensor_array.cpp` until they do. | Centering PID tuning assumes this baseline. Offset errors = wall hits. |
| **Shroud FOV** | Bench sweep: target approaches the sensor. Confirm reading snaps from real-distance to no-target right at the calculated angle. Lengthen tubes if needed. | Cross-talk = false walls = crashes. |

---

## 🎓 Design Decisions & Trade-Offs

### Why single battery, not separate 3.3 V rail?
Reduces weight and complexity. One 3S LiPo → MP1584EN buck (5 V) → XIAO LDO (3.3 V). Simpler, lighter, same efficiency.

### Why FOV shrouds, not just blinders?
Blinders are too broad — they reduce stray light but don't set a hard optical limit. A shroud aperture-tube is a precise mechanism that stops cross-talk and keeps the FOV narrow enough that no sensor "sees" its neighbor even at close range.

### Why ±30°, not ±45° diagonals?
For a single-path maze, early warning beats junction detection. 30° buys 1.7× more lead time into a corner. The original 45° came from classic micromouse design, where junctions matter. This event doesn't have them.

### Why v² braking, not a fixed deceleration?
Physics: `v² = v₀² + 2·a·d`. As the robot approaches a wall at different speeds, a v² profile keeps the wall-impact speed constant — the energy dissipation is quadratic with distance, not linear. This is how real-world brakes work.

---

## 🚀 Competition-Day Checklist

- [ ] Verify all three hardware unknowns (gear ratio, wheelbase, TOF offsets)
- [ ] Shroud FOV bench sweep test
- [ ] Battery at ≥11.7 V (full LiPo is 12.6 V)
- [ ] START button debounce working (press once, LED solid, ready)
- [ ] Telemetry OFF (–DCOMPETITION_BUILD)
- [ ] Wheels cleaned (silicone O-rings reduce slipping)
- [ ] Test re-arm cycle: FINISHED → press START → CALIBRATING → RUNNING
- [ ] Run 1: conservative cruise speed (350 mm/s) — **bank the points**
- [ ] Runs 2–3: raise `CRUISE_SPEED_MM_S` if run 1 was clean (weigh 5 pts/min reprogramming penalty)

**Key insight:** sections-first scoring. A bot that never touches at 0.45 m/s beats a bot that clips a wall at 0.6 m/s. Wall-touch avoidance outranks raw speed.

---

## 📚 Key Files to Know

- **[src/config.h](src/config.h)** — Every tuning knob in one place. Start here for tweaking PID gains, speed limits, distances.
- **[docs/WIRING.md](docs/WIRING.md)** — Pinout, power tree, I²C addresses, EMI countermeasures, shroud build recipe.
- **[docs/REPORT.md](docs/REPORT.md)** — Full technical analysis: rules vs. spec, architecture decisions, competition plan, all 7 design-brief discrepancies resolved.
- **[src/navigator.cpp](src/navigator.cpp)** — The heart: corridor centering, corner braking, pivot execution.

---

## 🔧 Future Improvements

- **IMU (BNO055):** Gyro-closed pivots immune to wheel slip. Huge accuracy upgrade.
- **Arc turns:** Carry 150+ mm/s through corners instead of stopping. Saves ~0.5s per corner.
- **Custom PCB:** Hand wiring + motor EMI is the #1 field-failure risk. A PCB with proper grounding and shielding changes everything.
- **Flash telemetry logging:** Log sensor data, encoder counts, PID outputs for post-run analysis without wireless.

---

## 📄 License

MIT

---

## 👤 Author

Built by **Amal Babu** for IEEE RAS SBC GECT, July 2026.

---

### 🎯 The Goal

**Fastest *and* cleanest run wins.**

*No walls touched. No restarts. Just pure algorithmic precision at speed.*

Compete hard. ⚡
