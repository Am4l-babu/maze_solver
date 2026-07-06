// ============================================================
// TEST 04 — SSD1306 0.96" I2C OLED (128x64) bring-up
//
// Actual intended role on the robot: setup/calibration screens and
// a persisted "total runtime" hour-meter. NOT a live in-run display —
// it is never updated while the robot is in STATE_RUNNING, so it can
// never compete with the TOF sensor sweep for I2C bus time during a
// timed run. This test's demo reflects that: a splash + test pattern
// at boot, then a slow (1 Hz) runtime counter — nothing high-frequency.
// ============================================================
#include <Arduino.h>
#include <Wire.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>
#include <Preferences.h>

#define PIN_SDA 5   // XIAO D4 / GPIO5 — shared main I2C bus
#define PIN_SCL 6   // XIAO D5 / GPIO6

#define SCREEN_W 128
#define SCREEN_H 64

// Most 0.96" blue SSD1306 boards are 0x3C; some clones ship as 0x3D.
Adafruit_SSD1306 display(SCREEN_W, SCREEN_H, &Wire, -1);
Preferences prefs;
uint8_t foundAddr = 0;

// Persisted lifetime "hour-meter" — survives power cycles via NVS flash.
// Written once per second here for the demo; a real deployment should
// throttle this further (e.g. every 60s) to reduce flash wear.
uint32_t totalRuntimeS = 0;
uint32_t sessionStartS = 0;

bool tryInit(uint8_t addr) {
    if (display.begin(SSD1306_SWITCHCAPVCC, addr)) {
        foundAddr = addr;
        return true;
    }
    return false;
}

void drawRuntimeScreen() {
    uint32_t sessionS = (millis() / 1000) - sessionStartS;
    uint32_t liveTotal = totalRuntimeS + sessionS;

    display.clearDisplay();
    display.setTextSize(1);
    display.setCursor(0, 0);
    display.println("MICRO-MAZE SOLVER");
    display.drawFastHLine(0, 10, SCREEN_W, SSD1306_WHITE);

    display.setCursor(0, 18);
    display.println("Total runtime:");
    display.setTextSize(2);
    display.setCursor(0, 28);
    display.printf("%luh%02lum%02lus\n", liveTotal / 3600, (liveTotal / 60) % 60, liveTotal % 60);

    display.setTextSize(1);
    display.setCursor(0, 52);
    display.printf("this session: %lus", sessionS);
    display.display();
}

void setup() {
    Serial.begin(115200);
    delay(300);
    Serial.println("[TEST] SSD1306 128x64 OLED — setup screens + runtime hour-meter");

    Wire.begin(PIN_SDA, PIN_SCL);
    Wire.setClock(400000);

    if (!tryInit(0x3C) && !tryInit(0x3D)) {
        Serial.println("[FAIL] no SSD1306 found at 0x3C or 0x3D. Check: "
                        "VCC->3.3V, GND, SDA->D4, SCL->D5.");
        while (true) delay(1000);
    }
    Serial.printf("[OK] SSD1306 found at 0x%02X\n", foundAddr);

    prefs.begin("mms", false);
    totalRuntimeS = prefs.getUInt("runtime_s", 0);
    sessionStartS = millis() / 1000;
    Serial.printf("[OK] loaded persisted runtime: %lu s (%luh%02lum%02lus)\n",
                  totalRuntimeS, totalRuntimeS / 3600, (totalRuntimeS / 60) % 60,
                  totalRuntimeS % 60);

    // --- splash screen ("setup" role #1) ---
    display.clearDisplay();
    display.setTextColor(SSD1306_WHITE);
    display.setTextSize(1);
    display.setCursor(0, 0);
    display.println("MICRO-MAZE SOLVER");
    display.println("PRIM.E Chapter II");
    display.setCursor(0, 30);
    display.setTextSize(2);
    display.println("OLED OK");
    display.display();
    delay(1500);

    // --- test pattern: border + 4 corner pixels, catches any
    //     row/column offset or mirrored-panel wiring issue ---
    display.clearDisplay();
    display.drawRect(0, 0, SCREEN_W, SCREEN_H, SSD1306_WHITE);
    display.fillCircle(4, 4, 2, SSD1306_WHITE);
    display.fillCircle(SCREEN_W - 5, 4, 2, SSD1306_WHITE);
    display.fillCircle(4, SCREEN_H - 5, 2, SSD1306_WHITE);
    display.fillCircle(SCREEN_W - 5, SCREEN_H - 5, 2, SSD1306_WHITE);
    display.setTextSize(1);
    display.setCursor(38, 28);
    display.println("PATTERN");
    display.display();
    delay(2000);

    Serial.println("[TEST] entering runtime-counter screen (updates once/sec, "
                    "not continuously — this is intentional, see README).");
}

void loop() {
    static uint32_t lastUpdateMs = 0;
    uint32_t now = millis();
    if (now - lastUpdateMs < 1000) return; // 1 Hz — this is not a live display
    lastUpdateMs = now;

    drawRuntimeScreen();

    // Persist the running total once per second for this demo. On the
    // real robot, throttle this to ~once/minute to limit flash wear.
    uint32_t sessionS = (now / 1000) - sessionStartS;
    prefs.putUInt("runtime_s", totalRuntimeS + sessionS);
}
