// ============================================================
// TEST 01 — Single VL53L0X (TOF200C) bring-up
// One bare sensor, direct-wired, no mux. Run this FIRST, before
// wiring the PCA9548A mux or the other four sensors — it proves
// the sensor itself, the 3.3V rail, and the I2C bus all work.
// ============================================================
#include <Arduino.h>
#include <Wire.h>
#include <Adafruit_VL53L0X.h>

#define PIN_SDA 5   // XIAO D4 / GPIO5
#define PIN_SCL 6   // XIAO D5 / GPIO6

Adafruit_VL53L0X tof;
bool sensorOk = false;

void setup() {
    Serial.begin(115200);
    delay(300);
    Serial.println("[TEST] single VL53L0X — direct wire, no mux, addr 0x29");

    Wire.begin(PIN_SDA, PIN_SCL);
    Wire.setClock(400000);

    sensorOk = tof.begin(0x29, false, &Wire, Adafruit_VL53L0X::VL53L0X_SENSE_HIGH_SPEED);
    if (!sensorOk) {
        Serial.println("[FAIL] sensor not found. Check: VIN->3.3V (NOT 5V), "
                        "GND, SDA->D4, SCL->D5, 4.7k pull-ups on the bus.");
        return;
    }
    tof.startRangeContinuous(20);
    Serial.println("[OK] sensor initialised. Printing distance every 100ms.");
    Serial.println("     Wave a hand toward it — reading should track 30-1200mm.");
}

void loop() {
    if (!sensorOk) {
        Serial.println("[FAIL] no sensor — halted.");
        delay(1000);
        return;
    }

    if (tof.isRangeComplete()) {
        uint16_t mm = tof.readRangeResult();
        bool outOfRange = (mm == 0 || mm > 2000);

        Serial.printf("%5u mm  ", mm);
        // crude serial bar-graph, 1 char per 20mm, capped at 60 chars
        int bars = outOfRange ? 0 : constrain(mm / 20, 0, 60);
        for (int i = 0; i < bars; i++) Serial.print('=');
        Serial.println(outOfRange ? "  (out of range)" : "");
    }
    delay(100);
}
