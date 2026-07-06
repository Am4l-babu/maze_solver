// ============================================================
// TEST 01 — Single VL53L0X bring-up + XSHUT pin check
// One bare sensor, direct-wired, no mux. Run this FIRST, before
// wiring the other four sensors — it proves the sensor itself,
// the 3.3V rail, the I2C bus, and the XSHUT line all work before
// test 02 chains all five together on one bus via XSHUT-driven
// re-addressing.
// ============================================================
#include <Arduino.h>
#include <Wire.h>
#include <Adafruit_VL53L0X.h>

#define PIN_SDA   5    // XIAO D4 / GPIO5
#define PIN_SCL   6    // XIAO D5 / GPIO6
#define PIN_XSHUT 42   // XIAO D11 / GPIO42 — direct-wired for this test only.
                        // On the real array (test 02) XSHUT is driven by the
                        // MCP23017 instead, since 5 lines don't fit on the
                        // XIAO's remaining spare GPIOs.

Adafruit_VL53L0X tof;
bool sensorOk = false;

bool initSensor() {
    return tof.begin(0x29, false, &Wire, Adafruit_VL53L0X::VL53L0X_SENSE_HIGH_SPEED);
}

void setup() {
    Serial.begin(115200);
    delay(300);
    Serial.println("[TEST] single VL53L0X — direct wire, no mux, addr 0x29");

    pinMode(PIN_XSHUT, OUTPUT);
    digitalWrite(PIN_XSHUT, HIGH);  // enabled
    delay(10);

    Wire.begin(PIN_SDA, PIN_SCL);
    Wire.setClock(400000);

    // --- XSHUT sanity check, before trusting it in the 5-sensor test ---
    Serial.println("[TEST] XSHUT check: shutting sensor down, expecting it to "
                    "disappear from the bus...");
    digitalWrite(PIN_XSHUT, LOW);
    delay(10);
    bool respondsWhileShutdown = initSensor();
    if (respondsWhileShutdown) {
        Serial.println("[FAIL] sensor still responded with XSHUT held LOW — "
                        "check the XSHUT wire is actually reaching the sensor "
                        "(not floating, not stuck high).");
    } else {
        Serial.println("[OK] sensor correctly silent while XSHUT is LOW.");
    }

    Serial.println("[TEST] re-enabling XSHUT, expecting the sensor to come back...");
    digitalWrite(PIN_XSHUT, HIGH);
    delay(10);
    sensorOk = initSensor();
    if (!sensorOk) {
        Serial.println("[FAIL] sensor not found after re-enabling XSHUT. Check: "
                        "VIN->3.3V (NOT 5V), GND, SDA->D4, SCL->D5, XSHUT->D11, "
                        "4.7k pull-ups on the bus.");
        return;
    }
    Serial.println("[OK] sensor re-initialised after XSHUT toggle — this pin "
                    "behaves as test 02 needs it to.");

    tof.startRangeContinuous(20);
    Serial.println("[OK] Printing distance every 100ms.");
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
