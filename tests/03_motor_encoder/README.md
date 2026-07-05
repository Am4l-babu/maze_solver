# Test 03 — Motor + Encoder Bring-Up (Forward / Back / Speed)

Drives each motor individually, then both together, then a short-brake
stop, then a PWM speed sweep — printing encoder-measured distance and
speed at every step. This is also where you empirically check the
**gear-ratio assumption** (100:1 vs 150:1 — the datasheet conflicts with
itself) using real numbers instead of guessing.

## ⚠️ Safety first

**Put the chassis on a stand so the wheels spin freely before running
this.** The sequence drives both motors automatically, including a
full-speed brake test. Don't run it on a table edge or in your hands.

## Wiring

| Component | Connects to |
|---|---|
| MCP23017 | XIAO D4/D5 (I²C, addr 0x20) |
| TB6612FNG PWMA/PWMB | XIAO D0/D1 |
| TB6612FNG AIN1/AIN2/BIN1/BIN2/STBY | MCP23017 GPA0–4 |
| TB6612FNG VMOT | Battery (direct) |
| TB6612FNG VCC | 3.3V rail |
| Left encoder A/B | XIAO D2/D3 |
| Right encoder A/B | XIAO D6/D7 (wired A/B swapped in code — right motor is mirrored) |
| Start button | XIAO D10 → GND (internal pull-up) |

## Run it

```sh
pio run -e test_motor_encoder -t upload -t monitor
```

Press the **START button** when prompted — the test won't move until you
do, matching the real robot's calibration-gate behavior.

## Expected output

```
MM_PER_COUNT = 0.0751  (gear ratio assumed 150:1 -- verify!)
Press START to begin the automatic test sequence...

-- Individual motor direction test --
  LEFT  forward          counts=  1840  dist=  138.2 mm  speed~=  138.2 mm/s
  LEFT  backward         counts= -1832  dist= -137.6 mm  speed~= -137.6 mm/s
  RIGHT forward          counts=  1855  dist=  139.3 mm  speed~=  139.3 mm/s
  RIGHT backward         counts= -1849  dist= -138.8 mm  speed~= -138.8 mm/s

-- Straight-line check (both motors together) --
  L=205.3 mm  R=201.9 mm  mismatch=1.7% (>10% suggests drive/friction imbalance)

-- Short-brake stopping distance --
  left wheel travelled 42.3 mm from full speed to brake engage+300ms

-- PWM speed sweep (left motor, open-loop) --
  PWM= 80                counts=   410  dist=   30.8 mm  speed~=   61.5 mm/s
  PWM=115                counts=   690  dist=   51.8 mm  speed~=  103.6 mm/s
  ...
[DONE] Sequence complete. Reset the board to run again.
```

## Reading the results

- **Forward/backward mismatch per motor** (e.g. forward=138mm but
  backward=-100mm) → check encoder wiring, or the motor's mechanical
  coupling (gear slip).
- **Straight-line mismatch >10%** → motors have different effective gear
  ratios, or one wheel has more rolling resistance — expect to need a
  per-side PID trim.
- **Gear ratio check**: the datasheet conflict is 100:1 vs 150:1. Manually
  rotate one wheel exactly 10 full turns by hand while watching the raw
  `counts` via a quick throwaway `Serial.println(encL.getCount())` (or
  just note the counts printed after a timed run and cross-check against
  the expected distance for a known number of wheel rotations). If actual
  counts are ~2/3 of what `MM_PER_COUNT` predicts, the real ratio is
  100:1, not 150:1 — update `src/config.h`'s `GEAR_RATIO`.
- **PWM sweep** — use this to find the *minimum* PWM that reliably
  overcomes static friction (the first row with nonzero speed). Anything
  commanded below that in the main firmware will stall the motor without
  moving.

## If it fails

- **`[FAIL] MCP23017 not found`** — check I²C wiring/address; if the
  LiDAR tests passed, the bus itself is fine, so recheck the MCP23017
  specifically.
- **Motor doesn't spin at all** — check STBY is being driven HIGH (active
  low = disabled), check VMOT has battery voltage, check TB6612 isn't in
  thermal shutdown from an earlier stall.
- **Encoder counts stay at 0** — check A/B wiring, check the encoder is
  actually powered, check for loose connector.

## Next step

Drive verified — you now have everything needed to flash the real robot
firmware (`pio run -e xiao_esp32s3`). Consider
[`tests/05_push_button`](../05_push_button/README.md) too if you haven't
already confirmed the start button independently.
