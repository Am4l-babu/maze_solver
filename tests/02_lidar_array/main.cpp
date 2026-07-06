// ============================================================
// TEST 02 — Full 5x VL53L0X array via XSHUT re-addressing
// (No PCA9548A mux — this sensor's XSHUT pin lets all five live
// on one I2C bus at once, each with a unique address.)
//
// Run this AFTER test 01 passes, including its XSHUT check —
// that proves the pin behaves correctly on one sensor before you
// trust the 5-way sequential bring-up below.
//
// Sequence: hold all 5 sensors in shutdown -> release one at a
// time -> each wakes at the default address 0x29 -> immediately
// move it to a unique address -> move on to the next. The last
// sensor doesn't need moving, since nothing else is left to
// collide with it at 0x29.
// ============================================================
#include <Arduino.h>
#include <Wire.h>
#include <Adafruit_MCP23X17.h>
#include <Adafruit_VL53L0X.h>

#define PIN_SDA       5     // XIAO D4 / GPIO5
#define PIN_SCL       6     // XIAO D5 / GPIO6
#define ADDR_MCP23017 0x20
#define NUM_TOF       5

// MCP23017 port B, pins 8-12 (GPB0-GPB4) drive XSHUT for TOF-0..4.
// Port A (pins 0-4) is reserved for motor direction/STBY on the real
// robot — kept clear here even though this test never touches a motor,
// so the pin map matches the final wiring exactly.
const uint8_t XSHUT_PIN[NUM_TOF] = {8, 9, 10, 11, 12};

// Channel -> mounted angle, matches src/config.h in the main firmware.
const char* SENSOR_NAME[NUM_TOF] = {
    "TOF-0 -90 LEFT ",
    "TOF-1 -30 DIAG-L",
    "TOF-2   0 FRONT ",
    "TOF-3 +30 DIAG-R",
    "TOF-4 +90 RIGHT ",
};

// Reassigned addresses. The last sensor keeps the power-on default —
// nothing else will claim 0x29 once every other sensor has moved off it.
const uint8_t NEW_ADDR[NUM_TOF] = {0x30, 0x31, 0x32, 0x33, 0x29};

Adafruit_MCP23X17 mcp;
Adafruit_VL53L0X tof[NUM_TOF];
bool ok[NUM_TOF] = {false, false, false, false, false};

void setup() {
    Serial.begin(115200);
    delay(300);
    Serial.println("[TEST] 5x VL53L0X via XSHUT re-addressing (no mux)");

    Wire.begin(PIN_SDA, PIN_SCL);
    Wire.setClock(400000);

    if (!mcp.begin_I2C(ADDR_MCP23017, &Wire)) {
        Serial.println("[FAIL] MCP23017 not found at 0x20 — check wiring.");
        while (true) delay(1000);
    }

    // Hold every sensor in shutdown first, so none of them answer at the
    // shared default address while we bring them up one at a time.
    for (uint8_t i = 0; i < NUM_TOF; i++) {
        mcp.pinMode(XSHUT_PIN[i], OUTPUT);
        mcp.digitalWrite(XSHUT_PIN[i], LOW);
    }
    delay(10);

    for (uint8_t i = 0; i < NUM_TOF; i++) {
        mcp.digitalWrite(XSHUT_PIN[i], HIGH);
        delay(10); // boot time before the sensor will ACK on the bus

        ok[i] = tof[i].begin(0x29, false, &Wire, Adafruit_VL53L0X::VL53L0X_SENSE_HIGH_SPEED);
        if (ok[i] && NEW_ADDR[i] != 0x29) {
            ok[i] = tof[i].setAddress(NEW_ADDR[i]);
        }
        Serial.printf("  %s -> 0x%02X : %s\n", SENSOR_NAME[i], NEW_ADDR[i],
                      ok[i] ? "OK" : "FAIL");
        if (ok[i]) tof[i].startRangeContinuous(20);
    }

    bool allOk = true;
    for (uint8_t i = 0; i < NUM_TOF; i++) allOk &= ok[i];
    if (!allOk) {
        Serial.println("[WARN] one or more sensors failed — check that sensor's "
                        "wiring/XSHUT line before trusting the full array. A "
                        "failed sensor here usually means its XSHUT never went "
                        "HIGH (bad MCP23017 pin) or a bad solder joint on VIN/GND.");
    }
    Serial.println("[TEST] streaming all 5 distances. Sweep a hand left-to-right "
                    "in front of the bumper — the reading should travel across "
                    "the columns L -> DL -> F -> DR -> R in order.");
}

uint16_t readOne(uint8_t idx) {
    if (!ok[idx]) return 0;
    if (!tof[idx].isRangeComplete()) return 0xFFFF; // not ready this cycle
    uint16_t mm = tof[idx].readRangeResult();
    return (mm == 0 || mm > 2000) ? 2000 : mm;
}

void loop() {
    uint16_t d[NUM_TOF];
    for (uint8_t i = 0; i < NUM_TOF; i++) d[i] = readOne(i);

    Serial.printf("L:%4u  DL:%4u  F:%4u  DR:%4u  R:%4u\n",
                  d[0], d[1], d[2], d[3], d[4]);
    delay(150);
}
