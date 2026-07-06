# 🧪 Hardware Bring-Up Test Suite

> Five standalone sketches. Five subsystems. Zero guessing about whether
> a wire is in the wrong hole before you trust the real firmware with it.

Every test here is a **self-contained PlatformIO environment** — it
doesn't depend on `src/`, so it keeps working even mid-refactor of the
main robot firmware. Flash one, watch the serial monitor, fix what's
wrong, move to the next. By the time you reach the last one, every
subsystem the robot depends on has been proven individually.

---

## 🗺️ The Order That Actually Makes Sense

```
  ① LiDAR single  ──▶  ② LiDAR array  ──▶  ⑤ Push button
        │                                        │
        └──────────────┬─────────────────────────┘
                        ▼
              ③ Motor + encoder
                        │
                        ▼
              🤖  flash real firmware
                        │
                        ▼
              ④ OLED  (optional, standalone)
```

Sensors before motors — if a sensor lies to you, you want to know
*before* the robot is moving and trusting that lie to steer. The button
test can happen any time; the OLED is entirely optional and not
currently wired into the main firmware at all.

---

## 📋 The Five Tests

| # | Test | Proves | Env name |
|---|------|--------|----------|
| 01 | [LiDAR — single](01_lidar_single/README.md) | One bare VL53L0X responds, before the mux is even wired | `test_lidar_single` |
| 02 | [LiDAR — array](02_lidar_array/README.md) | All 5 sensors + PCA9548A mux, angles map correctly | `test_lidar_array` |
| 03 | [Motor + encoder](03_motor_encoder/README.md) | Forward/back/speed on both motors, gear-ratio sanity check | `test_motor_encoder` |
| 04 | [OLED display](04_oled_display/README.md) | New SSD1306 128×64 status display | `test_oled` |
| 05 | [Push button](05_push_button/README.md) | Start-button wiring, debounce, press classification | `test_button` |

Run any of them:

```sh
pio run -e test_lidar_single  -t upload -t monitor
pio run -e test_lidar_array   -t upload -t monitor
pio run -e test_motor_encoder -t upload -t monitor
pio run -e test_oled          -t upload -t monitor
pio run -e test_button        -t upload -t monitor
```

Full pin-by-pin wiring for every component (including the ones not
covered by an individual test) lives in
[`docs/CONNECTIONS.md`](../docs/CONNECTIONS.md) — the single master
reference for every wire on the robot.

---

## 🧠 Why Standalone Sketches, Not Unit Tests

These aren't PlatformIO Unity-style pass/fail assertions — they're
**interactive bring-up sketches** you watch and reason about, because
"is the LiDAR seeing my hand at the right distance" and "did the wheel
actually spin the direction I told it to" are questions a human needs to
answer by looking, not questions a `TEST_ASSERT` macro can answer for
you. Each one prints exactly enough to make the answer obvious, and gets
out of the way otherwise.

Each folder is a real `main.cpp` + `README.md` pair — copy the folder
structure if you want to add a sixth test later (an IMU bring-up, once
one is wired in, would follow the exact same pattern).

---

## 🔧 Suggestions & Upgrades

Ideas that came directly out of *building* these tests — not
speculative, things that are one step away from being real:

### Near-term (cheap, high value)
- **Port the validated XSHUT sensor design into `src/`.** Tests 01/02
  prove all 5 VL53L0X sensors work directly on the bus with no mux —
  `src/sensor_array.cpp` still implements the older PCA9548A design.
  Once you're confident in the test rig, migrate `config.h` (drop
  `ADDR_PCA9548A`, add the XSHUT pin map + new addresses) and
  `sensor_array.cpp` (boot-time re-address sequence instead of
  per-read channel select) to match. One less physical component on
  the real robot once this lands.
- **Keep the OLED to setup/idle only, as tested.** Test 04 already
  reflects the real intended role — splash + test pattern at boot, then
  a 1Hz total-runtime hour-meter (persisted via NVS) — and is
  deliberately never updated during `STATE_RUNNING`. If you do wire it
  into `src/`, keep that constraint: call into it only from
  `IDLE`/`CALIBRATING`/`FINISHED`, never from the 100Hz control path.
- **Automate the gear-ratio check.** Test 03 already prints raw encoder
  counts alongside computed distance — a 10-second script that spins the
  wheel a commanded number of turns and computes the *actual* ratio
  automatically would remove the last manual step in that verification.
- **Add a 6th test: full-rig smoke test.** Once all five pass
  individually, a combined sketch that reads all sensors + both encoders
  + the button simultaneously (no motor movement) would catch the one
  class of bug individual tests can't: **I²C bus contention** between the
  continuous VL53L0X polling and the MCP23017 motor-direction/XSHUT
  writes when both are hammered at once — exactly the scenario the main
  firmware's dual-core split (`I2CGuard` in `src/i2c_bus.h`) was built
  to handle safely.

### Medium-term (real feature work)
- **IMU bring-up test**, once a BNO055/MPU6050 is wired to a spare
  MCP23017/GPIO pin — gyro-closed pivots are the biggest accuracy
  upgrade available for the 90° turns, per `docs/REPORT.md` §5.
- **Long-press on the start button → settings mode.** Test 05 already
  classifies short vs. long presses (600ms threshold) even though the
  main firmware only currently uses press *edges*. A long-press could
  gate something like "recalibrate TOF zero-offsets on the spot" without
  needing to re-flash.
- **PWM-sweep → auto-tuned minimum drive PWM.** Test 03's speed sweep
  finds the first PWM value that actually moves the wheel — worth baking
  that number into `src/config.h` as a `MIN_DRIVE_PWM` floor so the speed
  PID never wastes time commanding a PWM that can't overcome static
  friction.

### Long-term (bigger rework)
- **Custom PCB.** Every wiring-fault class these tests catch — a swapped
  SDA/SCL, a floating pin, a cold solder joint — mostly stops being
  possible once the hand-wiring is replaced. Still the single highest-
  leverage reliability upgrade available (see `docs/REPORT.md` §5).
- **Flash-based test logging.** Right now every test reports over
  serial only. Logging each bring-up run's pass/fail + raw numbers to
  flash would build a maintenance history across a season of
  re-wiring/repairs — useful for catching a connector that's slowly
  degrading before it fails mid-competition.

---

## ✅ Wiring Checklist (all five tests)

Before touching any of these, run down
[`docs/CONNECTIONS.md`](../docs/CONNECTIONS.md)'s checklist — every I²C
device on 3.3V (never 5V), pull-ups present, common star ground, motor
terminal caps installed. Skipping this is how a $6 sensor test turns
into a "why did my ESP32 die" afternoon.
