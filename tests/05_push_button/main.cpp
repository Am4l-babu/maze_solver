// ============================================================
// TEST 05 — Push button bring-up
// Verifies wiring, debounce, and short/long-press classification
// for the START button before it's trusted by the state machine.
// ============================================================
#include <Arduino.h>

#define PIN_BTN 9   // XIAO D10 / GPIO9 — button to GND, internal pull-up
#define PIN_LED 8   // XIAO D9  / GPIO8 — onboard-visible status LED

const uint32_t DEBOUNCE_MS   = 50;
const uint32_t LONG_PRESS_MS = 600;

bool     lastRaw = false;
bool     stableState = false;
uint32_t lastChangeMs = 0;
uint32_t pressStartMs = 0;
uint32_t pressCount = 0;

void setup() {
    Serial.begin(115200);
    delay(300);
    pinMode(PIN_BTN, INPUT_PULLUP);
    pinMode(PIN_LED, OUTPUT);
    Serial.println("[TEST] push button — press and release the START button");
    Serial.println("[TEST] expecting: PRESS / RELEASE events, no chatter from bounce");
}

void loop() {
    bool raw = (digitalRead(PIN_BTN) == LOW);  // active low
    uint32_t now = millis();

    if (raw != lastRaw) {
        lastChangeMs = now;
        lastRaw = raw;
    }

    if ((now - lastChangeMs) > DEBOUNCE_MS && raw != stableState) {
        stableState = raw;
        digitalWrite(PIN_LED, stableState ? HIGH : LOW);

        if (stableState) {
            pressStartMs = now;
            pressCount++;
            Serial.printf("[%6lu ms] PRESS   (#%lu)\n", now, pressCount);
        } else {
            uint32_t held = now - pressStartMs;
            Serial.printf("[%6lu ms] RELEASE (held %lu ms) -> %s\n",
                          now, held, held >= LONG_PRESS_MS ? "LONG" : "short");
        }
    }
}
