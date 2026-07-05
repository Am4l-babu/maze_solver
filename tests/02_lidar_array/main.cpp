// ============================================================
// TEST 02 — Full 5x VL53L0X array through the PCA9548A mux
// Run this AFTER test 01 passes on at least one loose sensor.
// Verifies: mux channel switching, all 5 sensors respond at
// their shared address 0x29 (isolated per-channel), and the
// physical mounting order matches the intended angle map.
// ============================================================
#include <Arduino.h>
#include <Wire.h>
#include <Adafruit_VL53L0X.h>

#define PIN_SDA        5     // XIAO D4 / GPIO5
#define PIN_SCL        6     // XIAO D5 / GPIO6
#define ADDR_PCA9548A  0x70
#define ADDR_VL53L0X   0x29
#define NUM_TOF        5

// Channel -> mounted angle, matches src/config.h in the main firmware.
const char* CHANNEL_NAME[NUM_TOF] = {
    "TOF-0 -90 LEFT ",
    "TOF-1 -30 DIAG-L",
    "TOF-2   0 FRONT ",
    "TOF-3 +30 DIAG-R",
    "TOF-4 +90 RIGHT ",
};

Adafruit_VL53L0X tof[NUM_TOF];
bool ok[NUM_TOF] = {false, false, false, false, false};

void selectChannel(uint8_t ch) {
    Wire.beginTransmission(ADDR_PCA9548A);
    Wire.write(1 << ch);
    Wire.endTransmission();
}

void setup() {
    Serial.begin(115200);
    delay(300);
    Serial.println("[TEST] 5x VL53L0X via PCA9548A mux (addr 0x70)");

    Wire.begin(PIN_SDA, PIN_SCL);
    Wire.setClock(400000);

    for (uint8_t i = 0; i < NUM_TOF; i++) {
        selectChannel(i);
        ok[i] = tof[i].begin(ADDR_VL53L0X, false, &Wire,
                              Adafruit_VL53L0X::VL53L0X_SENSE_HIGH_SPEED);
        Serial.printf("  CH%u %s : %s\n", i, CHANNEL_NAME[i], ok[i] ? "OK" : "FAIL");
        if (ok[i]) tof[i].startRangeContinuous(20);
    }

    bool allOk = true;
    for (uint8_t i = 0; i < NUM_TOF; i++) allOk &= ok[i];
    if (!allOk) {
        Serial.println("[WARN] one or more channels failed — check that channel's "
                        "sensor wiring before trusting the full array.");
    }
    Serial.println("[TEST] streaming all 5 distances. Sweep a hand left-to-right in "
                    "front of the bumper — you should see the reading travel across "
                    "the columns L -> DL -> F -> DR -> R in order.");
}

uint16_t readOne(uint8_t idx) {
    if (!ok[idx]) return 0;
    selectChannel(idx);
    if (!tof[idx].isRangeComplete()) return 0xFFFF;  // not ready this cycle
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
