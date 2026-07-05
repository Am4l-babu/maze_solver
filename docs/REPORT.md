# Micro-Maze Solver — Technical Report

**Event:** IEEE RAS SBC GECT, PRIM.E Chapter II — Maze Solver (July 2026)
**Robot:** 120 × 125 × 55 mm differential-drive, XIAO ESP32S3, 5× VL53L0X

---

## 1. Executive summary

The original design brief assumed a classic micromouse-style maze
(exploration → flood fill → fast run). **The published competition rules
describe a different event**, and the firmware architecture was changed
accordingly:

| Assumption in design brief | Actual PRIM.E rule | Consequence |
|---|---|---|
| Decision junctions, dead ends | **Single path, no dead ends, no junctions** (Comp. §7) | Flood fill is unnecessary for scoring; demoted to fallback module |
| Goal = maze center | Start top-left → finish bottom-right (Comp. §11) | Finish detection = corridor fully opens, not a center cell |
| ~180 mm cells | **230 mm track width** (Gen. §6, Comp. §3) | More clearance; centered gap is ~55 mm/side for a 120 mm bot |
| 50–80 mm walls, 55 mm bot too tall | **Walls up to 10 cm; bot limit 15×15×10 cm** (Gen. §5, Comp. §4) | 55 mm height is legal — no redesign needed |
| Points for reaching center | **10 sections; wall touch forfeits from that section on** (Comp. §8–9) | Wall-touch avoidance outranks raw speed |
| — | Best of 3 runs, 4 min each, 12 min total | Re-arm flow after finish; conservative run 1, faster runs 2–3 |
| — | No wireless once run starts (Comp. §13) | `COMPETITION_BUILD` strips telemetry; WiFi/BT never initialized |

**Strategy that follows:** ranking is (sections cleared, then time). A bot
that never touches a wall at 0.45 m/s beats a bot that clips a wall at
0.6 m/s. The control system therefore prioritizes centering accuracy, brakes
early on a v² profile into corners, and only then maximizes straightaway
speed.

---

## 2. System architecture

### 2.1 Hardware

- **MCU:** XIAO ESP32S3 — 240 MHz dual-core, hardware PCNT quadrature
  decoding, LEDC PWM. Ample headroom for a 100 Hz control loop.
- **Sensing:** five VL53L0X time-of-flight sensors at −90/−45/0/+45/+90°,
  all default address 0x29, isolated by a **PCA9548A I²C mux** (one channel
  each). Continuous-ranging mode at 20 ms period; a full 5-sensor sweep
  costs ~1 ms of bus time per loop. 15° printed blinders on the front three
  prevent cross-talk.
- **Drive:** TB6612FNG. PWM (20 kHz) from the MCU; direction pins and STBY
  on an **MCP23017** expander to free GPIOs. Short-brake mode (both inputs
  low, PWM high) gives hard active braking for corner entry.
- **Power:** single 3S LiPo → MP1584EN buck (5 V) → XIAO LDO (3.3 V).
  **All I²C peripherals are powered from 3.3 V** — the ESP32-S3 is not 5 V
  tolerant (design brief issue #4, resolved).

### 2.2 Firmware (100 Hz loop)

```
sense (TOF sweep, PCNT encoders, battery ADC)
  → state machine (IDLE / CALIBRATING / RUNNING / FINISHED / FAULT)
    → navigator (phase machine: FOLLOW → BRAKE_FOR_TURN → PIVOT → EXIT_TURN)
      → per-wheel speed PID → voltage-compensated PWM → TB6612FNG
```

- **Inner loop:** per-motor speed PID on encoder mm/s. Derivative on
  measurement with low-pass filtering (at 450 counts/rev and 100 Hz the
  speed quantization is ~30 mm/s — raw D would chatter).
- **Outer loop:** steering PID on wall-distance error. Both walls:
  `(left − right)/2`. One wall (pre-corner): hold 55 mm offset. Diagonal
  sensors add an approach-angle term that reacts before the 90° pair.
- **Corner handling:** front-distance trigger → v² braking profile
  (`v = √(2·a·d)`, a = 1.5 m/s²) → short-brake → encoder-counted in-place
  pivot (arc = π/4 × wheelbase) → 60 mm straight blind exit before the side
  sensors are trusted again (they sweep across the corner opening).
- **Speed governor:** cruise speed is scaled down by centering error, so
  the bot is fast only when it has earned it.
- **Robustness:** 500 ms hardware watchdog; battery monitoring with a
  low-voltage fault stop; PWM voltage compensation (11.1 V / V_batt) so PID
  tuning holds as the pack sags across three back-to-back runs.

### 2.3 Fallback: classic maze mode

`maze_map.*` implements wall mapping + BFS flood fill + straight-preferring
move selection, selectable with `MODE_SINGLE_PATH 0`. Kept because rule 16
says final scoring criteria arrive only 24 h before the event — if the
"single path" claim changes, the bot can still explore and solve.

---

## 3. Resolved design-brief issues

| # | Issue | Resolution |
|---|---|---|
| 1 | Gear ratio 100:1 vs 150:1 | Firmware assumes 150:1; **must verify** by rolling 10 wheel turns and checking counts (procedure in README). One constant to change. |
| 2 | Top speed 0.45 vs 0.67 m/s | Cruise set to 0.45 m/s — inside both estimates. Raise after track testing; sections-before-speed scoring makes this low-risk. |
| 3 | 55 mm height vs walls | Non-issue under real rules (10 cm bot limit). |
| 4 | 5 V I²C vs 3.3 V MCU | All I²C devices powered from the 3.3 V rail (wiring doc). |
| 5 | "Separate 3.3 V battery" | Confirmed single-battery; power tree documented. |
| 6 | TB6612 thermal | 1.2 A/ch continuous vs 1 A/motor stall — acceptable; short-brake pulses are brief. Heatsink recommended on the mid deck. |
| 7 | Point-turn clearance | 230 mm track (not 180 mm) leaves ~55 mm/side; in-place pivots of a 120×125 mm bot (diagonal ≈ 173 mm < 230 mm) are safe when centered. Pivot is entered only after braking to near-zero while centered. |

---

## 4. Competition-day plan

1. **2 h setup window:** verify gear-ratio constant, TOF offsets on the
   real walls (surface reflectivity matters), battery full, run 1 at
   conservative cruise (350 mm/s).
2. **Run structure (12 min / 3 × 4 min):** run 1 clean and slow — bank
   sections. If clean, raise `CRUISE_SPEED_MM_S` (this is *reprogramming*,
   note the 5 pts/min penalty tradeoff — only worth it if run 1 dropped
   sections). Runs 2–3 progressively faster; best of 3 counts.
3. **Failure policy:** the bot never reverses along the track (no dead ends
   exist); a FAULT stop is preferred over a wall strike since points are
   kept up to the preceding section.
4. **Pre-run checklist:** START button re-arm cycle tested, telemetry build
   flag OFF, WiFi/BT off, wheels cleaned (silicone O-rings), battery
   ≥ 11.7 V.

---

## 5. Future improvements

- **IMU (BNO055/MPU6050)** on a spare mux channel — gyro-closed pivots are
  immune to wheel slip, the main pivot-accuracy risk.
- **Arc (swept) cornering** — carrying 150 mm/s through corners instead of
  stopping to pivot would save ~0.5 s/corner; requires the IMU first.
- **Custom PCB** to replace hand wiring — EMI from N20 brush arcing is the
  single most likely source of field failures; the interim mitigations
  (motor caps, ferrites, star ground, twisted encoder pairs) are mandatory.
- **Flash telemetry logging** for post-run analysis without wireless.
