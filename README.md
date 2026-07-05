# Micro-Maze Solver Robot

Firmware for a custom autonomous maze-solving robot built for the
**IEEE RAS SBC GECT — PRIM.E Chapter II Maze Solver Competition (July 2026)**.

> **Key rules insight:** the PRIM.E maze is a **single continuous path** —
> no dead ends, no decision junctions (Competition Guidelines §7). Any wall
> touch forfeits the current section's points. So this firmware is built
> around **fast, wall-touch-free corridor centering with 90° corner turns**,
> not classic exploration. A flood-fill module is kept as a fallback
> (`MODE_SINGLE_PATH` in `src/config.h`).

## Hardware

| Subsystem | Part |
|---|---|
| MCU | Seeed Studio XIAO ESP32S3 (dual-core LX7 @ 240 MHz) |
| Ranging | 5× TOF200C (VL53L0X) at −90°, −45°, 0°, +45°, +90° |
| I²C mux | PCA9548A @ 0x70 (isolates the five identical 0x29 sensors) |
| I/O expander | MCP23017 @ 0x20 (motor direction + STBY) |
| Motor driver | TB6612FNG (short-brake used for active braking) |
| Motors | 2× N20 12 V metal-gear, quadrature encoders (PCNT-decoded) |
| Power | 3S LiPo 1000 mAh → MP1584EN buck (5 V) → XIAO LDO (3.3 V) |

Full wiring, pin map, and power tree: [docs/WIRING.md](docs/WIRING.md).
Design analysis and competition strategy: [docs/REPORT.md](docs/REPORT.md).

## Repo layout

```
src/
├── main.cpp           100 Hz control loop + top-level state machine
├── config.h           pin map, physical constants, all tuning knobs
├── pid_controller.*   generic PID (anti-windup, filtered D-on-measurement)
├── sensor_array.*     PCA9548A mux + 5× VL53L0X continuous ranging
├── motor_driver.*     TB6612FNG via LEDC PWM + MCP23017 direction pins
├── encoder.*          hardware PCNT quadrature decoding, mm/s speeds
├── navigator.*        corridor centering, v² braking, encoder pivots
├── state_machine.h    IDLE → CALIBRATING → RUNNING → FINISHED / FAULT
└── maze_map.*         flood-fill fallback for classic mazes (unused in PRIM.E mode)
```

## Build & flash

Requires [PlatformIO](https://platformio.org/) (CLI or VS Code extension).

```sh
pio run                 # build
pio run -t upload       # flash over USB-C
pio device monitor      # 115200 baud telemetry
```

For competition day, build with telemetry stripped:

```sh
pio run -e xiao_esp32s3 -DCOMPETITION_BUILD
```

Also disable WiFi/BT pairing tools — rule 13 prohibits any wireless link
once the run starts.

## Before first run — verify on hardware

1. **Gear ratio** — vendor listings conflict (100:1 vs 150:1). Roll the bot
   exactly 10 wheel turns by hand and compare encoder counts against
   `COUNTS_PER_REV` in `config.h`.
2. **Motor direction & encoder polarity** — positive PWM on both motors must
   drive forward and produce increasing counts.
3. **Wheelbase** — measure center-to-center on the assembled chassis and
   update `WHEELBASE_MM` (pivot accuracy depends on it).
4. **TOF offsets** — place the bot centered in a 230 mm jig; both side
   sensors should read ~55 mm. Trim with `SensorArray::setOffset`.
5. **Battery divider** — check ADC reading against a multimeter.

## Operation

1. Power on → LED triple-blink, then slow blink (IDLE).
2. Place at the start (top-left corner), press START.
3. LED solid for 1 s (calibration/settle), then the run begins.
4. Fast blink = finished. Press START again to re-arm for the next round.
5. Very fast blink = fault (low battery / sensor loss) — check serial log.
