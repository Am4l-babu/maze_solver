// ============================================================
// Micro-Maze Solver — main control loop (100 Hz)
// IEEE RAS SBC GECT — PRIM.E Chapter II Maze Solver, July 2026
// ============================================================
#include <Arduino.h>
#include <Wire.h>
#include <Adafruit_MCP23X17.h>
#include <esp_task_wdt.h>

#include "config.h"
#include "pid_controller.h"
#include "sensor_array.h"
#include "motor_driver.h"
#include "encoder.h"
#include "navigator.h"
#include "state_machine.h"

Adafruit_MCP23X17 mcp;
SensorArray   sensors;
MotorDriver   motors;
EncoderSystem encoders;
Navigator     navigator;
StateMachine  fsm;

PIDController speedPID_L(SPEED_KP, SPEED_KI, SPEED_KD, -255, 255);
PIDController speedPID_R(SPEED_KP, SPEED_KI, SPEED_KD, -255, 255);

static uint32_t lastLoopMs = 0;
static float    batteryV   = 11.1f;

float readBatteryVoltage() {
    // 30k/10k divider; 12-bit ADC, 3.3 V reference. IIR-smoothed.
    static float filt = 11.1f;
    float v = analogReadMilliVolts(PIN_BATT_ADC) / 1000.0f * BATT_DIVIDER_RATIO;
    filt += 0.05f * (v - filt);
    return filt;
}

void blink(uint8_t n, uint16_t ms) {
    for (uint8_t i = 0; i < n; i++) {
        digitalWrite(PIN_LED, HIGH); delay(ms);
        digitalWrite(PIN_LED, LOW);  delay(ms);
    }
}

void enterFault(const char* why) {
    Serial.printf("[FAULT] %s\n", why);
    motors.shortBrake();
    delay(150);
    motors.coast();
    motors.standby(true);
    fsm.setState(STATE_FAULT);
}

void setup() {
    Serial.begin(115200);
    pinMode(PIN_START_BTN, INPUT_PULLUP);
    pinMode(PIN_LED, OUTPUT);
    analogSetPinAttenuation(PIN_BATT_ADC, ADC_11db);

    Wire.begin(PIN_SDA, PIN_SCL);
    Wire.setClock(I2C_FREQ_HZ);

    if (!mcp.begin_I2C(ADDR_MCP23017, &Wire)) {
        Serial.println("[INIT] MCP23017 not found — check wiring");
        while (true) blink(2, 100);
    }
    motors.begin(&mcp);
    encoders.begin();

    if (!sensors.begin()) {
        Serial.println("[INIT] one or more TOF sensors failed");
        while (true) blink(3, 100);
    }
    navigator.begin(&encoders);

    // Hardware watchdog: resets the MCU if the loop hangs mid-run.
    esp_task_wdt_config_t wdt = { .timeout_ms = 500,
                                  .idle_core_mask = 0,
                                  .trigger_panic = true };
    esp_task_wdt_reconfigure(&wdt);
    esp_task_wdt_add(nullptr);

    fsm.setState(STATE_IDLE);
    Serial.println("[INIT] ready — press START");
    blink(3, 60);
}

void loop() {
    esp_task_wdt_reset();

    uint32_t now = millis();
    if (now - lastLoopMs < LOOP_PERIOD_MS) return;
    float dt = (now - lastLoopMs) / 1000.0f;
    lastLoopMs = now;

    // ---- sense ------------------------------------------------
    sensors.readAll();
    encoders.update(dt);
    batteryV = readBatteryVoltage();
    motors.setBatteryVoltage(batteryV);

    bool startEdge = fsm.startPressed(digitalRead(PIN_START_BTN) == LOW);

    // ---- state machine -----------------------------------------
    switch (fsm.getState()) {

    case STATE_IDLE:
        motors.coast();
        digitalWrite(PIN_LED, (now / 500) & 1);   // slow blink = ready
        if (startEdge) fsm.setState(STATE_CALIBRATING);
        break;

    case STATE_CALIBRATING:
        if (batteryV < BATT_LOW_V) { enterFault("battery low at start"); break; }
        encoders.zero();
        navigator.reset();
        speedPID_L.reset();
        speedPID_R.reset();
        motors.standby(false);
        digitalWrite(PIN_LED, HIGH);
        if (fsm.timeInState() > 1000) {           // 1 s settle, hands clear
            digitalWrite(PIN_LED, LOW);
            fsm.setState(STATE_RUNNING);
        }
        break;

    case STATE_RUNNING: {
        if (batteryV < BATT_CRIT_V) { enterFault("battery critical"); break; }

        navigator.plan(sensors.getData(),
                       MODE_SINGLE_PATH ? CRUISE_SPEED_MM_S : EXPLORE_SPEED_MM_S);

        if (navigator.phase() == Navigator::FINISHED) {
            motors.shortBrake();
            fsm.setState(STATE_FINISHED);
            break;
        }
        if (navigator.wantsShortBrake()) {
            motors.shortBrake();
            speedPID_L.reset();
            speedPID_R.reset();
            break;
        }

        float pwmL = speedPID_L.compute(navigator.leftTarget(),  encoders.getLeftSpeed(),  dt);
        float pwmR = speedPID_R.compute(navigator.rightTarget(), encoders.getRightSpeed(), dt);
        motors.setBoth(pwmL, pwmR);
        break;
    }

    case STATE_FINISHED:
        motors.coast();
        digitalWrite(PIN_LED, (now / 120) & 1);   // fast blink = done
        if (startEdge) fsm.setState(STATE_IDLE);  // re-arm for next round
        break;

    case STATE_FAULT:
        digitalWrite(PIN_LED, (now / 60) & 1);
        if (startEdge) { motors.standby(false); fsm.setState(STATE_IDLE); }
        break;

    default:
        fsm.setState(STATE_IDLE);
        break;
    }

    // ---- telemetry (disable BEFORE competition: rule 13 bans wireless,
    // and USB serial stalls the loop if the host buffers fill) ----------
#if !defined(COMPETITION_BUILD)
    static uint32_t lastTel = 0;
    if (now - lastTel >= 200) {
        lastTel = now;
        const SensorData& s = sensors.getData();
        Serial.printf("st=%d ph=%d bat=%.2f L=%.0f R=%.0f | %0.f %0.f %0.f %0.f %0.f\n",
                      fsm.getState(), navigator.phase(), batteryV,
                      encoders.getLeftSpeed(), encoders.getRightSpeed(),
                      s.left, s.diagL, s.front, s.diagR, s.right);
    }
#endif
}
