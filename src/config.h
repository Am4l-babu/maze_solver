#pragma once
// ============================================================
// config.h — Pin map, physical constants, tuning parameters
// Micro-Maze Solver — IEEE RAS SBC GECT PRIM.E Chapter II (July 2026)
// ============================================================

#include <Arduino.h>

// ------------------------------------------------------------
// COMPETITION MODE
// The PRIM.E rules define a SINGLE-PATH maze: no dead ends, no
// decision junctions. Primary strategy is fast corridor
// centering + corner turns. Flood-fill exploration is kept as
// a compile-time fallback for classic mazes / practice.
// ------------------------------------------------------------
#define MODE_SINGLE_PATH 1   // 1 = PRIM.E rules, 0 = classic flood-fill maze

// ---------------- XIAO ESP32S3 pin map ----------------------
// XIAO silk  GPIO   Function
#define PIN_PWM_L      1     // D0  — TB6612 PWMA (left motor)
#define PIN_PWM_R      2     // D1  — TB6612 PWMB (right motor)
#define PIN_ENC_LA     3     // D2  — left encoder ch A  (PCNT)
#define PIN_ENC_LB     4     // D3  — left encoder ch B  (PCNT)
#define PIN_SDA        5     // D4  — I2C SDA (shared bus)
#define PIN_SCL        6     // D5  — I2C SCL (shared bus)
#define PIN_ENC_RA     43    // D6  — right encoder ch A (PCNT)
#define PIN_ENC_RB     44    // D7  — right encoder ch B (PCNT)
#define PIN_BATT_ADC   7     // D8  — battery divider (30k/10k)
#define PIN_LED        8     // D9  — status LED / buzzer
#define PIN_START_BTN  9     // D10 — start button (pull-up, active low)

// ---------------- I2C addresses ------------------------------
#define ADDR_MCP23017  0x20
#define ADDR_PCA9548A  0x70
#define ADDR_VL53L0X   0x29  // all five share this — isolated by mux

#define I2C_FREQ_HZ    400000UL

// ---------------- PCA9548A channels --------------------------
#define CH_TOF_LEFT    0     // -90°
#define CH_TOF_DIAG_L  1     // -30° (shallow, not -45°: buys more corner
                             // lead-time in this narrow single-path track
                             // — see docs/WIRING.md "FOV-limiting shroud")
#define CH_TOF_FRONT   2     //   0°
#define CH_TOF_DIAG_R  3     // +30°
#define CH_TOF_RIGHT   4     // +90°
#define NUM_TOF        5

// ---------------- MCP23017 pins (port A) ---------------------
#define MCP_AIN1       0     // left motor dir 1
#define MCP_AIN2       1     // left motor dir 2
#define MCP_BIN1       2     // right motor dir 1
#define MCP_BIN2       3     // right motor dir 2
#define MCP_STBY       4     // TB6612 standby (active low)

// ---------------- Physical constants --------------------------
// !! VERIFY on hardware: gear ratio is listed as both 100:1 and
// 150:1 in vendor docs. Measure counts over 10 wheel turns.
#define GEAR_RATIO          150.0f
#define ENCODER_PPR         3.0f           // pre-gearbox, per channel edge set
#define COUNTS_PER_REV      (GEAR_RATIO * ENCODER_PPR * 4.0f) // x4 quadrature
#define WHEEL_DIAMETER_MM   43.0f
#define WHEEL_CIRC_MM       (PI * WHEEL_DIAMETER_MM)
#define MM_PER_COUNT        (WHEEL_CIRC_MM / COUNTS_PER_REV)
#define WHEELBASE_MM        95.0f          // measure on assembled chassis!

// ---------------- Track geometry (PRIM.E rules) ---------------
#define TRACK_WIDTH_MM      230.0f         // wall-to-wall corridor
#define ROBOT_WIDTH_MM      120.0f
// Distance each side sensor reads when perfectly centered:
#define CENTER_SIDE_DIST_MM ((TRACK_WIDTH_MM - ROBOT_WIDTH_MM) * 0.5f) // ~55
#define WALL_PRESENT_MM     160.0f         // below this a side wall exists
#define OPENING_MM          200.0f         // above this the side is open

// ---------------- Speeds & motion profile ----------------------
#define EXPLORE_SPEED_MM_S   250.0f
#define CRUISE_SPEED_MM_S    450.0f        // fast-run straightaway
#define TURN_APPROACH_MM_S   150.0f
#define MAX_DECEL_MM_S2      1500.0f
#define TURN_TRIGGER_MM      110.0f        // front distance to start a turn
#define STOP_THRESHOLD_MM    45.0f         // short-brake engage distance

// ---------------- PID gains (tune on hardware) ------------------
#define SPEED_KP   0.9f
#define SPEED_KI   6.0f
#define SPEED_KD   0.02f

#define STEER_KP   2.2f
#define STEER_KI   0.0f
#define STEER_KD   0.8f

// ---------------- Loop timing ----------------------------------
#define LOOP_PERIOD_MS  10                 // 100 Hz control loop

// ---------------- Battery monitor -------------------------------
#define BATT_DIVIDER_RATIO  4.0f           // 30k + 10k
#define BATT_LOW_V          10.2f          // 3.4 V/cell warning
#define BATT_CRIT_V         9.6f           // 3.2 V/cell — stop the run

// ---------------- Maze grid (classic fallback mode) --------------
#define MAZE_COLS   16
#define MAZE_ROWS   16
#define CELL_SIZE_MM 250.0f                // adjust to revealed track
