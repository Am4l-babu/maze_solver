# Test 05 — Push Button (Start Button) Bring-Up

Verifies wiring, debounce, and short/long-press classification for the
START button before the state machine trusts it. Mirrors the exact
50ms debounce window used in `src/state_machine.h`.

## Wiring

| Button leg | Connects to |
|---|---|
| Leg 1 | XIAO D10 (GPIO9) |
| Leg 2 | XIAO GND |

No external resistor needed — the test enables the XIAO's internal
pull-up (`INPUT_PULLUP`), so the pin reads HIGH when idle and LOW when
pressed.

## Run it

```sh
pio run -e test_button -t upload -t monitor
```

## Expected output

```
[TEST] push button — press and release the START button
[TEST] expecting: PRESS / RELEASE events, no chatter from bounce
[  1204 ms] PRESS   (#1)
[  1389 ms] RELEASE (held 185 ms) -> short
[  3011 ms] PRESS   (#2)
[  3810 ms] RELEASE (held 799 ms) -> LONG
```

The onboard status LED (D9) mirrors the button state in real time — LED
on while held, off while released — for a visual check without needing
the serial monitor.

## What to check

- **Exactly one PRESS per physical press** — if you see multiple
  PRESS/RELEASE pairs from a single press, the switch is bouncing more
  than the 50ms debounce window covers; a hardware debounce cap
  (0.1µF across the switch) fixes mechanically noisy switches.
- **No spontaneous presses** — if PRESS events appear with nothing
  touched, check for a floating pin (bad ground connection) or EMI
  pickup from a nearby motor lead.
- **Long-press threshold** (600ms) — matches nothing in the main
  firmware today (which only uses press *edges*, not hold duration) but
  is included here as a hook for a future "hold 3s to enter a settings
  menu" style feature — see Suggestions in `tests/README.md`.

## Next step

All five subsystems verified — you're ready to flash the real robot
firmware: `pio run -e xiao_esp32s3 -t upload`.
